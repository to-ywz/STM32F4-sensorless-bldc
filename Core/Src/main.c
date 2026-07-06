/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "open_loop_vf.h"
#include "svpwm.h"
#include "foc_math.h"
#include "foc_hw_pwm.h"
#include "comm_uart.h"
#include "comm_uart_stm32.h"
#include "comm_vofa.h"
#include "comm_cmd.h"
#include "comm_scope.h"
#include "app_debug.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VDC             12.0f       /* 母线电压 (V) */
#define PWM_FREQ        16000       /* PWM 频率 (Hz) */
#define PWM_PERIOD      5249        /* TIM1 ARR 值 */
#define DEAD_TIME_US    1.0f        /* 死区时间 (us) */

#define VF_TARGET_FREQ  17.0f       /* 目标频率 (Hz) */
#define VF_ACCEL        2.0f        /* 加速度 (Hz/s) */
#define VF_RATIO        0.05f       /* V/f 比 (V/Hz) */
#define VF_V_MAX        5.93f       /* 最大电压 (V) = Vdc/√3 */
#define VF_V_MIN        0.2f        /* 最小电压 (V) */

#define DEBUG_UART_RX_DMA_SIZE   64U
#define DEBUG_UART_RX_RING_SIZE  128U
#define DEBUG_UART_TX_RING_SIZE  512U
#define DEBUG_UART_TX_DMA_SIZE   64U
#define DEBUG_FREQ_MIN_HZ        0.0f
#define DEBUG_FREQ_MAX_HZ        30.0f

/* TODO: VOFA Scope 直出会导致 VOFA+ 卡死，后续改为分频输出或触发捕获。 */
#define VOFA_OUTPUT_MODE              VOFA_MODE_NORMAL
#define VOFA_NORMAL_PERIOD_MS         1U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static open_loop_vf_t   vf;            /* V/f 控制器实例 */
static svpwm_output_t   svpwm;         /* SVPWM 实例 */
static hw_pwm_instance_t hw_pwm;       /* PWM 硬件实例 */

static comm_uart_t       debug_uart;      /* 调试串口抽象对象 */
static comm_uart_stm32_t debug_uart_drv;  /* STM32 串口适配对象 */
static comm_vofa_t       debug_vofa;      /* VOFA+ JustFloat 输出对象 */
static comm_cmd_t        debug_cmd;       /* 串口文本命令解析对象 */
static comm_scope_t      debug_scope;     /* 调试示波输出对象 */
static app_debug_t       app_debug;       /* 应用层调试模板对象 */

static uint8_t debug_uart_rx_dma_buf[DEBUG_UART_RX_DMA_SIZE];
static uint8_t debug_uart_rx_ring_buf[DEBUG_UART_RX_RING_SIZE];
static uint8_t debug_uart_tx_ring_buf[DEBUG_UART_TX_RING_SIZE];
static uint8_t debug_uart_tx_dma_buf[DEBUG_UART_TX_DMA_SIZE];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void vf_self_check(void);
static void debug_comm_init(void);
static void debug_comm_task(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  MX_TIM6_Init();
  MX_USART1_UART_Init();
  MX_ADC3_Init();
  /* USER CODE BEGIN 2 */

  /* 调试串口初始化：USART1 + DMA + VOFA+ JustFloat */
  debug_comm_init();

  /* 控制模块初始化 */
  open_loop_vf_init(&vf, VF_TARGET_FREQ, VF_ACCEL, VF_RATIO, VF_V_MAX, VF_V_MIN);
  /* 预定位参数: 0.5V, 持续 0.5s (保守值，待调试) */
  open_loop_vf_set_align(&vf, 0.5f, 0.5f);

  svpwm_init(&svpwm, VDC, PWM_FREQ, PWM_PERIOD);

  hw_pwm_config_t pwm_cfg = {
      .htim              = &htim1,
      .timer_clk_hz      = 168000000U,
      .period_ticks      = PWM_PERIOD,
      .adc_trigger_ticks = (PWM_PERIOD + 1U) / 2U,
      .dead_time_us      = DEAD_TIME_US,
      .v_dc              = VDC,
  };
  hw_pwm_init(&hw_pwm, &pwm_cfg);

  /* 自检: 验证 PWM 硬件通路 */
  vf_self_check();

  /* 启动 TIM1 计数器（不开更新中断） */
  HAL_TIM_Base_Start(&htim1);

  /* 启动 ADC 注入转换，TIM1 CC4 触发 */
  HAL_ADCEx_InjectedStart_IT(&hadc1);

  /* 使能 PWM 输出 (SD = 高) */
  hw_pwm_enable(&hw_pwm);

  /* ADC 偏置校准和母线检查完成后，调用 open_loop_vf_start(&vf) 启动电机。
     当前阶段: 保持 STOP 状态，不自动启动。
     TODO: 需要实现 ADC 偏置校准和母线电压检查。 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    debug_comm_task();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
 * @brief  自检：关闭 SD → 发固定波形 → 关闭输出 → 开启 SD
 *
 * 仅用于调试验证 PWM 硬件通路，后续可替换为其他流程。
 */
static void vf_self_check(void)
{
    /* 1. 关闭驱动器 (SD = 低) */
    hw_pwm_set_sd(0);

    /* 2. 设置 50% 占空比（三相对称，观测波形用） */
    pwm_output_t test_pwm = {
        .freq   = PWM_FREQ,
        .period = PWM_PERIOD,
        .cmp_a  = PWM_PERIOD / 2,
        .cmp_b  = PWM_PERIOD / 2,
        .cmp_c  = PWM_PERIOD / 2,
    };
    hw_pwm_set_duty(&hw_pwm, &test_pwm);

    /* 3. 使能 PWM 输出，示波器观测 */
    hw_pwm_enable(&hw_pwm);
    HAL_Delay(2000);

    /* 4. 关闭输出，停止 ADC 中断 */
    hw_pwm_disable(&hw_pwm);
    HAL_ADCEx_InjectedStop_IT(&hadc1);
    HAL_Delay(100);

    /* 5. 开启 SD */
    hw_pwm_set_sd(1);
}

/**
 * @brief 初始化调试串口输出链路
 *
 * 当前使用 USART1，发送侧为 ringbuffer + DMA，接收侧为 DMA + IDLE。
 * VOFA+ 上位机需要选择 JustFloat 协议，波特率 3000000。
 */
static void debug_comm_init(void)
{
    comm_uart_stm32_config_t uart_cfg = {
        .huart        = &huart1,
        .rx_dma_buf   = debug_uart_rx_dma_buf,
        .rx_dma_size  = DEBUG_UART_RX_DMA_SIZE,
        .rx_ring_buf  = debug_uart_rx_ring_buf,
        .rx_ring_size = DEBUG_UART_RX_RING_SIZE,
        .tx_ring_buf  = debug_uart_tx_ring_buf,
        .tx_ring_size = DEBUG_UART_TX_RING_SIZE,
        .tx_dma_buf   = debug_uart_tx_dma_buf,
        .tx_dma_size  = DEBUG_UART_TX_DMA_SIZE,
    };
    comm_cmd_config_t cmd_cfg = {
        .uart        = &debug_uart,
        .user        = &app_debug,
        .start       = app_debug_cmd_start,
        .stop        = app_debug_cmd_stop,
        .set_freq    = app_debug_cmd_set_freq,
        .freq_min_hz = DEBUG_FREQ_MIN_HZ,
        .freq_max_hz = DEBUG_FREQ_MAX_HZ,
    };
    comm_scope_config_t scope_cfg = {
        .vofa             = &debug_vofa,
        .mode             = VOFA_OUTPUT_MODE,
        .normal_period_ms = VOFA_NORMAL_PERIOD_MS,
    };
    app_debug_config_t app_debug_cfg = {
        .vf    = &vf,
        .svpwm = &svpwm,
        .cmd   = &debug_cmd,
        .scope = &debug_scope,
    };

    if (comm_uart_stm32_init(&debug_uart, &debug_uart_drv, &uart_cfg) < 0) {
        Error_Handler();
    }

    comm_vofa_init(&debug_vofa, &debug_uart);

    if (comm_uart_start_rx(&debug_uart) < 0) {
        Error_Handler();
    }

    if (comm_scope_init(&debug_scope, &scope_cfg) < 0) {
        Error_Handler();
    }

    if (app_debug_init(&app_debug, &app_debug_cfg) < 0) {
        Error_Handler();
    }

    if (comm_cmd_init(&debug_cmd, &cmd_cfg) < 0) {
        Error_Handler();
    }
}

/**
 * @brief 调试通信主循环任务
 */
static void debug_comm_task(void)
{
    app_debug_poll(&app_debug, HAL_GetTick());
}

/**
 * @brief  ADC 注入转换完成回调
 *
 * TIM1 CC4 触发 ADC 注入转换，转换完成后进入此回调。
 * 每个 PWM 周期执行一次控制链路:
 *   open_loop_vf_step → foc_inv_park → svpwm_update → hw_pwm_set_duty
 *
 * @param  hadc : ADC 句柄
 */
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance != ADC1)
        return;

    /* 读取注入转换结果（当前未使用，预留电流反馈） */
    (void)HAL_ADCEx_InjectedGetValue(hadc, ADC_INJECTED_RANK_1);
    (void)HAL_ADCEx_InjectedGetValue(hadc, ADC_INJECTED_RANK_2);
    (void)HAL_ADCEx_InjectedGetValue(hadc, ADC_INJECTED_RANK_3);

    float dt = 1.0f / (float)PWM_FREQ;     /* 控制周期 62.5μs */

    /* V/f 步进: 根据状态机更新频率、电压、角度 */
    open_loop_vf_step(&vf, dt);

    if (open_loop_vf_get_state(&vf) == OPEN_LOOP_VF_STATE_STOP) {
        /* STOP 状态: 输出零矢量 (50% 占空比，线电压平均为零) */
        pwm_output_t zero = {
            .freq   = PWM_FREQ,
            .period = PWM_PERIOD,
            .cmp_a  = PWM_PERIOD / 2,
            .cmp_b  = PWM_PERIOD / 2,
            .cmp_c  = PWM_PERIOD / 2,
        };
        hw_pwm_set_duty(&hw_pwm, &zero);

        app_debug_on_pwm_update(&app_debug, &zero);
    } else {
        /* ALIGN / RAMP / RUN: 正常控制链路 */
        float v_alpha, v_beta;
        foc_inv_park(vf.v_out, 0.0f, vf.theta_e, &v_alpha, &v_beta);
        svpwm_update(&svpwm, v_alpha, v_beta);
        hw_pwm_set_duty(&hw_pwm, &svpwm.pwm);

        app_debug_on_pwm_update(&app_debug, &svpwm.pwm);
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
