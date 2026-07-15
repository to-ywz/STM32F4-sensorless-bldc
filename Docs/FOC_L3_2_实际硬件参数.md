# FOC L3.2 实际硬件参数

本文档记录当前工程中已经落到代码和 CubeMX 配置里的真实硬件参数。

原则：当旧调研文档、口头记录、原理图理解和工程代码不一致时，当前阶段先以 `Foc_svpwm_VF.ioc`、`Core/Inc/main.h`、`Core/Src/tim.c`、`Core/Src/adc.c`、`Core/Src/main.c` 为准；后续用示波器和实物连线再反向修正文档。

## 1. 板级外设边界

目标是逐步把 FOC 算法库和板级外设适配层隔离。

当前建议边界：

| 层级 | 当前文件 | 职责 | 是否依赖 STM32 HAL |
|---|---|---|---|
| 算法库 | `foc/Src/svpwm.c` | αβ 电压到三相 PWM 比较值 | 否 |
| 算法库 | `foc/Src/foc_math.c` | Clarke/Park/InvPark、矢量限幅 | 否 |
| 算法库 | `foc/Src/pid.c` | PID/PI 控制 | 否 |
| 算法库 | `foc/Src/open_loop_vf.c` | 开环 V/f 状态机 | 否 |
| 板级适配 | `foc/Src/foc_hw_pwm.c` | TIM1 CCR 写入、互补 PWM 启停、死区、SD 引脚 | 是 |
| 应用层 | `Core/Src/main.c` | 参数配置、初始化顺序、控制中断链路 | 是 |

后续可将 `foc_hw_pwm.c` 重命名或拆分为类似 `pwm_hw_stm32.c` / `pwm_hw_stm32.h` 的板级外设接口。当前先不改名，避免在硬件验证前引入无关变更。

## 2. 主控与时钟

| 项目 | 当前值 | 来源 |
|---|---:|---|
| MCU 系列 | STM32F4 | `Core/Inc/stm32f4xx_hal_conf.h` / CubeMX 工程 |
| SYSCLK | 168 MHz | `SystemClock_Config()` |
| TIM1 时钟 | 168 MHz | `Core/Src/main.c` 中 `timer_clk_hz = 168000000U` |
| APB2 分频 | HCLK / 2 | `SystemClock_Config()` |
| TIM1 预分频 | 0 | `Core/Src/tim.c` |

TIM1 位于 APB2。APB2 分频不为 1 时，定时器时钟为 APB2 时钟的 2 倍，因此当前 TIM1 时钟为 168 MHz。

## 3. PWM 参数

| 项目 | 当前值 |
|---|---:|
| PWM 定时器 | TIM1 |
| PWM 模式 | 中心对齐 `TIM_COUNTERMODE_CENTERALIGNED1` |
| PWM 通道 | CH1/CH1N、CH2/CH2N、CH3/CH3N |
| ADC 触发通道 | TIM1 CH4，无输出 |
| TIM1 Prescaler | 0 |
| TIM1 ARR / `PWM_PERIOD` | 5249 |
| TIM1 RepetitionCounter | 1 |
| PWM 频率 | 16 kHz |
| 死区时间 | 0.5 us（当前测试配置） |
| PWM 输出极性 | CH/CHN 均为高电平有效 |
| ARR/CCR 预装载 | ARR 预装载开启；CCR 由 HAL PWM 配置管理 |

频率推导：

```text
f_pwm = f_tim / (2 * (ARR + 1))
      = 168 MHz / (2 * (5249 + 1))
      = 16 kHz
```

控制周期：

```text
dt = 1 / 16000 = 62.5 us
```

当前 `Core/Src/main.c` 中 `PWM_FREQ` 应保持为 `16000`，否则 V/f 状态机的 `dt` 会和真实 PWM 周期不一致。

## 4. PWM 引脚映射

当前 CubeMX / 代码中的实际 TIM1 引脚如下。

| 电机相 | 上桥臂 | 下桥臂 | TIM 通道 | 说明 |
|---|---|---|---|---|
| U | PE9 | PE8 | TIM1_CH1 / TIM1_CH1N | `cmp_a -> CCR1` |
| V | PE11 | PE10 | TIM1_CH2 / TIM1_CH2N | `cmp_b -> CCR2` |
| W | PE13 | PE12 | TIM1_CH3 / TIM1_CH3N | `cmp_c -> CCR3` |

注意：旧调研文档中曾记录过 PA8/PA7 等 PWM 引脚；当前工程实际使用 PE8~PE13。后续以示波器和实物连线确认后，再统一旧文档。

## 5. 驱动器使能

| 功能 | 当前引脚 | GPIO 标签 | 当前逻辑 |
|---|---|---|---|
| 驱动器 SD / 使能 | PA5 | `DRIVER_SD` | 高电平使能，低电平关闭 |

当前实现：

| 函数 | 行为 |
|---|---|
| `hw_pwm_init()` | 初始化后将 SD 拉低，关闭驱动器 |
| `hw_pwm_enable()` | 启动 PWM/互补 PWM/CH4 后，将 SD 拉高 |
| `hw_pwm_disable()` | 先将 SD 拉低，再停止 PWM |
| `hw_pwm_set_sd(enable)` | 直接控制 SD 引脚 |

注意：旧调研文档中曾记录 SD 为 PG10；当前工程实际为 PA5。

## 6. ADC 采样

### 6.1 三相电流

当前 ADC1 注入通道用于三相电流采样。

| 信号 | 引脚 | ADC | 通道 | 注入 Rank |
|---|---|---|---:|---:|
| IU | PA3 | ADC1 | IN3 | 1 |
| IV | PA4 | ADC1 | IN4 | 2 |
| IW | PA6 | ADC1 | IN6 | 3 |

触发源：

| 项目 | 当前值 |
|---|---|
| 触发源 | TIM1 CC4 |
| 触发边沿 | Rising |
| CH4 比较值 | `ADC_TRIGGER_TICKS = 3937` |
| CH4 模式 | PWM2，无实际输出 |

当前三相测试使用 50% 占空比，TIM1 为中心对齐计数，CH1~CH3 使用 PWM1。
在该条件下，`CCR=2625` 位于上下管切换附近，容易把 ADC 采样放在开关瞬态；
CH4 已调整为 `3/4 ARR = 3937`，位于低侧导通区中部。该固定值只适用于当前
50% 静态测试，后续进入 SVPWM 后需要根据占空比动态选择有效采样窗口。

当前 `HAL_ADCEx_InjectedConvCpltCallback()` 已读取 ADC1 注入 Rank 1~3，并按原理图参数换算为三相电流后送入 VOFA+；电流零点仍未校准，暂不用于过流保护和电流环。

### 6.2 相电压与母线电压

当前 ADC3 注入通道配置如下。根据配套原理图，PF6~PF8 是三相相电压（BEMF）反馈，PF9 是母线电压反馈。

| 信号 | 引脚 | ADC | 通道 | 注入 Rank |
|---|---|---|---:|---:|
| BEMF_U_ADC | PF6 | ADC3 | IN4 | 1 |
| BEMF_V_ADC | PF7 | ADC3 | IN5 | 2 |
| BEMF_W_ADC | PF8 | ADC3 | IN6 | 3 |
| VBUS_ADC | PF9 | ADC3 | IN7 | 4 |

ADC3 与 ADC1 使用相同的 TIM1 CC4 触发源。`main.c` 已启动 ADC3 注入中断，并在回调中保存四路原始值；应用层再完成电压换算并送入 VOFA+。

原理图中三相 BEMF 和母线电压的换算关系为：

```text
Motor_EMFU = Motor_U / 37
Motor_EMFV = Motor_V / 37
Motor_EMFW = Motor_W / 37
Motor_VBUS_ADC = POWER / 37 + 1.24 V
Motor_EMF*_ADC = Motor_EMF* + 1.24 V
```

当前 ADC 为 12 位，VDDA 通过 ADC1 注入 Rank 4 的内部 VREFINT 动态计算：

```text
VDDA = VREFINT_CAL × VREFINT_CAL_V / VREFINT_ADC原始值
ADC电压 = ADC原始值 × VDDA / 4095
实际相电压或 BEMF = (ADC电压 - 1.24) × 37
实际母线电压 = (ADC电压 - 1.24) × 37
```

VOFA+ 正常模式新增通道：

| 通道 | 含义 | 单位 |
|---:|---|---|
| 10 | U 相 BEMF / 相电压 | V |
| 11 | V 相 BEMF / 相电压 | V |
| 12 | W 相 BEMF / 相电压 | V |
| 13 | 母线电压 | V |
| 14 | U 相 ADC 引脚电压 | V |
| 15 | V 相 ADC 引脚电压 | V |
| 16 | W 相 ADC 引脚电压 | V |
| 17 | VBUS ADC 引脚电压 | V |
| 18 | 由 VREFINT 计算的实时 VDDA | V |
| 19 | VREFINT 原始 ADC 值 | count |

三相电流诊断通道：

| 通道 | 含义 | 单位 |
|---:|---|---|
| 0 | U 相电流 | A |
| 1 | V 相电流 | A |
| 2 | W 相电流 | A |
| 3 | U 相电流 ADC 电压 | V |
| 4 | V 相电流 ADC 电压 | V |
| 5 | W 相电流 ADC 电压 | V |

原理图给出的电流采样关系为 `Motor_I*_ADC = (Vinp - Vinn) × 8 + 1.24 V`，当前按 `0.02 Ω` 采样电阻和增益 `8` 换算：

```text
I = (ADC电压 - current_offset_v) / (0.02 × 8)
```

当前 `feature/current-calibration` 分支启动时会在驱动器关闭状态下采集 2048 组 ADC1 三相电流样本，分别生成 U/V/W 零点偏置；该标定结果仅保存在运行时 RAM，尚未接入过流保护和电流环。

原理图标注 `Motor_VBUS_ADC = POWER / 37 + 1.24 V`。但当前实测母线为 12.3 V、PF9 ADC 引脚为 1.77~1.78 V，而理论值应为约 1.572 V，存在以下待排查差异：

```text
12.3 / 37 + 1.24 ≈ 1.572 V
```

当前代码仍按原理图使用 ×37。该差异解决前，不应将经验比例用于过压保护；需要复核板上 VBUS 分压电阻、PF9 网络和 ADC 参考电压。

## 7. 当前 V/f 调试参数

| 参数 | 当前值 | 说明 |
|---|---:|---|
| `VDC` | 12.0 V | 当前母线电压假定值 |
| `VF_TARGET_FREQ` | 17.0 Hz | 目标电频率 |
| `VF_ACCEL` | 2.0 Hz/s | 频率爬坡斜率 |
| `VF_RATIO` | 0.05 V/Hz | V/f 比 |
| `VF_V_MAX` | 5.93 V | 当前电压矢量限幅 |
| `VF_V_MIN` | 0.2 V | 低频补偿 |
| 预定位电压 | 0.5 V | `open_loop_vf_set_align()` |
| 预定位时间 | 0.5 s | `open_loop_vf_set_align()` |

这组参数偏保守，适合低压、限流条件下继续验证 PWM 和开环 V/f。

## 8. 启动流程

当前 `main.c` 的初始化流程：

1. 初始化 HAL、时钟、GPIO、DMA、TIM1、ADC1、TIM6、USART1、ADC3。
2. 初始化 V/f、SVPWM、PWM 硬件适配层。
3. 执行 `vf_self_check()`：SD 关闭、输出 50% 占空比波形、短暂开启 PWM 供示波器观察、再关闭。
4. 启动 TIM1 Base。
5. 启动 ADC1 和 ADC3 注入中断，触发源均为 TIM1 CC4。
6. 使能 PWM 输出。
7. 调用 `open_loop_vf_start(&vf)`，进入 ALIGN/RAMP/RUN 状态机。
8. ADC1 注入转换完成回调中执行控制链路：

```text
open_loop_vf_step()
    -> foc_inv_park()
    -> svpwm_update()
    -> hw_pwm_set_duty()
```

## 9. 待确认 / 待改进

- 用示波器确认 PE9/PE8、PE11/PE10、PE13/PE12 的互补波形、频率、死区和相序。
- 用万用表或示波器确认 PA5 的 SD 使能极性是否与驱动板实际一致。
- 旧文档中 PA8/PA7、PG10 等记录需要在硬件确认后统一修正。
- 补 ADC1 三相电流零点校准：空载、PWM 关闭或零矢量时采样，记录 offset。
- 已接入 ADC3 三相 BEMF、母线电压采样启停和电压换算；后续需要通过实测电压校验偏置和比例。
- 将板级 PWM 适配层逐步改成统一接口，例如 `pwm_hw_stm32`，让算法库不依赖 STM32 HAL。
- 后续如硬件允许，优先考虑将故障信号接入 TIM1 BKIN，实现硬件级关断。
