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
#include "app_measurement.h"
#include "motor_fault.h"
#include "stm32f4xx_ll_adc.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// TODO: 这些宏定义是否需要放到配置文件中，方便后续的参数调整和维护，或者后期修改为变量，便于在运行时进行调试和优化，减少代码的硬编码，提高系统的灵活性和可配置性。
#define VDC             12.0f       /* 母线电压 (V) */
#define PWM_FREQ        16000       /* PWM 频率 (Hz) */
#define PWM_PERIOD      5249        /* TIM1 ARR 值 */
#define ADC_TRIGGER_TICKS ((PWM_PERIOD * 3U + 2U) / 4U) /* 50% PWM 导通区中部 */
#define DEAD_TIME_US    0.5f        /* 死区时间 (us) */

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

#define ADC_VREF                 3.0f
#define ADC_MAX_COUNT            4095.0f
#define BEMF_OFFSET_V            1.26f
#define BEMF_SCALE               37.0f
#define VBUS_OFFSET_V            1.26f
#define VBUS_SCALE               37.0f       /* 原理图：POWER / 37 + 1.24V */

/* TODO: VOFA Scope 直出会导致 VOFA+ 卡死，后续改为分频输出或触发捕获。 */
#define VOFA_OUTPUT_MODE              VOFA_MODE_NORMAL
#define VOFA_NORMAL_PERIOD_MS         1U
#define CURRENT_ZERO_CALIBRATION_SAMPLES  2048U
#define CURRENT_ZERO_CALIBRATION_TIMEOUT_MS  250U
#define SOFTWARE_OVERCURRENT_LIMIT_A     5.0f
#define SOFTWARE_OVERCURRENT_TRIP_COUNT  2U
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
static motor_fault_t     motor_fault;     /* 电机控制故障锁存对象 */

static app_measurement_t app_measurement;   // TODO: 这个名字太长，是否需要重构较短的，更加简洁

typedef struct {
    uint8_t sector_applied;
    uint8_t sector_next;
} pwm_sector_state_t;   // TODO: 是否为测试代码需要删除

static pwm_sector_state_t pwm_sector_state; // TODO: 是否为测试代码需要删除

// 是否要将这部分的 DMA buffer 和 ring buffer 放到 comm_uart_stm32_t 结构体中，减少全局变量的使用
static uint8_t debug_uart_rx_dma_buf[DEBUG_UART_RX_DMA_SIZE];
static uint8_t debug_uart_rx_ring_buf[DEBUG_UART_RX_RING_SIZE];
static uint8_t debug_uart_tx_ring_buf[DEBUG_UART_TX_RING_SIZE];
static uint8_t debug_uart_tx_dma_buf[DEBUG_UART_TX_DMA_SIZE];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// 初始化函数是否需要集成在统一的初始化文件中，减少 main.c 的代码量
static void debug_comm_init(void);
static void debug_comm_task(void);
static int current_zero_calibration(void);
static void power_stage_disable(void);
static void raise_control_fault(motor_fault_code_t code);
static int debug_power_stage_set(void *user, uint8_t enable);
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

  app_measurement_config_t measurement_cfg = {
      .adc_vref       = ADC_VREF,
      .adc_max_count  = ADC_MAX_COUNT,
      .vrefint_cal_raw = *VREFINT_CAL_ADDR,
      .vrefint_cal_v  = (float)VREFINT_CAL_VREF / 1000.0f,
      .current_offset_v = 1.27f,
      .current_shunt_ohm = 0.02f,
      .current_amplifier_gain = 8.0f,
      .software_overcurrent_limit_a = SOFTWARE_OVERCURRENT_LIMIT_A,
      .software_overcurrent_trip_count = SOFTWARE_OVERCURRENT_TRIP_COUNT,
      .bemf_offset_v  = BEMF_OFFSET_V,
      .bemf_scale     = BEMF_SCALE,
      .vbus_offset_v  = VBUS_OFFSET_V,
      .vbus_scale     = VBUS_SCALE,
  };
  if (app_measurement_init(&app_measurement, &measurement_cfg) < 0) {
      Error_Handler();
  }

  /* USER CODE BEGIN 2 */

  motor_fault_init(&motor_fault);

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
      .adc_trigger_ticks = ADC_TRIGGER_TICKS,
      .dead_time_us      = DEAD_TIME_US,
      .v_dc              = VDC,
  };
  hw_pwm_init(&hw_pwm, &pwm_cfg);

  /* 启动 TIM1 计数器（不开更新中断） */
  HAL_TIM_Base_Start(&htim1);

  /* 上电默认关闭功率级；仅由串口 start 命令显式使能。 */
  hw_pwm_disable(&hw_pwm);  // TODO: 这里直接接触硬件层，理论上应该通过应用层接口来控制功率级的使能和关闭，避免直接操作硬件导致潜在的安全问题。

  /* 启动 ADC 注入转换，TIM1 CC4 触发 */ // TODO: 这部分是否考虑形成函数，方便后续的 ADC 配置和启动流程的复用
  HAL_ADCEx_InjectedStart_IT(&hadc1);
  HAL_ADCEx_InjectedStart_IT(&hadc3);

  /* 驱动器保持关闭，只启动 CH4 触发 ADC，完成三相电流零点标定。 */
  if (current_zero_calibration() < 0) {
      Error_Handler();
  }

  /* 零点校准完成后才允许软件过流保护参与运行判断。 */
  app_measurement_set_overcurrent_enabled(&app_measurement, 1U);

  /* 三相电流零点校准已经完成；母线电压保护仍未接入。
     当前阶段保持 STOP 状态，功率级关闭，等待串口 start 命令。 */
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
        .measurement = &app_measurement,
        .fault = &motor_fault,
        .power_stage_set = debug_power_stage_set,
        .power_stage_user = NULL,
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
 * @brief 串口启动流程使用的功率级控制回调。
 * @param user 未使用，保留回调接口扩展性。
 * @param enable 1: 启动 PWM 并使能 SD；0: 关闭 PWM 并拉低 SD。
 * @return 0 表示目标状态已设置。
 */
static int debug_power_stage_set(void *user, uint8_t enable)
{
    (void)user;

    if (enable != 0U) {
        hw_pwm_enable(&hw_pwm);
        return hw_pwm_is_enabled(&hw_pwm) != 0U ? 0 : -1;
    }

    power_stage_disable();
    return hw_pwm_is_enabled(&hw_pwm) == 0U ? 0 : -1;
}

/**
 * @brief 关闭功率级并停止控制状态。
 * @note 该函数只负责动作，不清除故障锁存，异常状态下串口 start 命令不生效。
 */
static void power_stage_disable(void)
{
    open_loop_vf_stop(&vf);
    hw_pwm_power_stage_disable(&hw_pwm);
    hw_pwm_motor_outputs_disable(&hw_pwm);
    pwm_sector_state.sector_applied = 0U;
    pwm_sector_state.sector_next = 0U;
}

/**
 * @brief 保存控制现场并锁存调制故障。
 */
static void raise_control_fault(motor_fault_code_t code)
{
    motor_fault_context_t context = {
        .svpwm_fault = svpwm.fault,
        .sector      = pwm_sector_state.sector_applied,
        .v_alpha     = svpwm.v_alpha,
        .v_beta      = svpwm.v_beta,
        .cmp_a       = svpwm.pwm.cmp_a,
        .cmp_b       = svpwm.pwm.cmp_b,
        .cmp_c       = svpwm.pwm.cmp_c,
        .tick_ms     = HAL_GetTick(),
    };

    motor_fault_raise(&motor_fault, code, &context);
}

/**
 * @brief 在驱动器关闭时采集三相电流零点。
 *
 * 仅打开 TIM1 CH4 作为 ADC 触发源，不打开三相 PWM 和驱动器 SD。
 */
static int current_zero_calibration(void)
{
    uint32_t start_tick;

    app_measurement_start_current_zero_calibration(
        &app_measurement, CURRENT_ZERO_CALIBRATION_SAMPLES);

    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4) != HAL_OK) {
        return -1;
    }

    start_tick = HAL_GetTick();
    while (!app_measurement_is_current_zero_calibration_done(
               &app_measurement)) {
        debug_comm_task();
        if ((uint32_t)(HAL_GetTick() - start_tick) >=
            CURRENT_ZERO_CALIBRATION_TIMEOUT_MS) {
            HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
            return -2;
        }
    }

    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);

    return app_measurement_apply_current_zero_calibration(
        &app_measurement);
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
    if (hadc->Instance == ADC3) {
        uint16_t bemf_u_raw = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_1);
        uint16_t bemf_v_raw = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_2);
        uint16_t bemf_w_raw = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_3);
        uint16_t vbus_raw = (uint16_t)HAL_ADCEx_InjectedGetValue(
            hadc, ADC_INJECTED_RANK_4);

        app_measurement_update(&app_measurement,
                               bemf_u_raw,
                               bemf_v_raw,
                               bemf_w_raw,
                               vbus_raw);
        return;
    }

    if (hadc->Instance != ADC1)
        return;

    /* 本次 ADC 采样属于上一轮已经写入并生效的 PWM。 */
    pwm_sector_state.sector_applied = pwm_sector_state.sector_next;

    /* 读取三相电流原始值；测量模块会按扇区选择两相并重构第三相。 */
    uint16_t current_u_raw = (uint16_t)HAL_ADCEx_InjectedGetValue(
        hadc, ADC_INJECTED_RANK_1);
    uint16_t current_v_raw = (uint16_t)HAL_ADCEx_InjectedGetValue(
        hadc, ADC_INJECTED_RANK_2);
    uint16_t current_w_raw = (uint16_t)HAL_ADCEx_InjectedGetValue(
        hadc, ADC_INJECTED_RANK_3);
    uint16_t vrefint_raw = (uint16_t)HAL_ADCEx_InjectedGetValue(
        hadc, ADC_INJECTED_RANK_4);
    app_measurement_update_current(&app_measurement,
                                   pwm_sector_state.sector_applied,
                                   current_u_raw,
                                   current_v_raw,
                                   current_w_raw);
    app_measurement_update_vrefint(&app_measurement, vrefint_raw);

    /* 故障锁存后保留 ADC 采集和通信观察，但不再执行控制计算。 */
    if (motor_fault_is_latched(&motor_fault) != 0U) {
        return;
    }

    if (app_measurement_check_overcurrent(&app_measurement,
                                          pwm_sector_state.sector_applied,
                                          current_u_raw,
                                          current_v_raw,
        current_w_raw) != 0U) {
        /* 过流故障锁存：关闭功率级，禁止控制链路继续运行。 */
        raise_control_fault(MOTOR_FAULT_OVERCURRENT);
        power_stage_disable();
        return;
    }

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

        /* 安全零矢量不是最终保护动作，必须切断功率级并锁存故障。 */
        if (svpwm.fault != SVPWM_FAULT_NONE) {
            raise_control_fault(MOTOR_FAULT_MODULATION);
            power_stage_disable();
            return;
        }

        hw_pwm_set_duty(&hw_pwm, &svpwm.pwm);

        /* 比较值写入后，当前计算的扇区成为下一次采样对应的扇区。 */
        pwm_sector_state.sector_next = svpwm.sector_next;

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
