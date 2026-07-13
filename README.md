# FOC SVPWM V/F 控制库

基于 STM32F4 的磁场定向控制（FOC）SVPWM 库，用于无刷电机的 V/F 控制。

## 项目概述

本项目旨在实现一个完整的 FOC 控制库，包括：
- SVPWM（空间矢量脉宽调制）生成
- V/F 控制算法
- PWM 硬件接口层
- 电流采样与保护

## 硬件平台

| 项目 | 规格 |
|---|---|
| MCU | STM32F4 (T100) |
| 驱动板 | 野火无刷电机驱动板（V1R2） |
| PWM 定时器 | TIM1（高级定时器，6路互补输出） |
| PWM 频率 | 16kHz |
| 电流采样 | 三相电流（IA/IB/IC）+ 母线电压 |

## 目录结构

```
├── Core/               # STM32 HAL 核心代码
│   ├── Inc/            # 头文件
│   └── Src/            # 源文件
├── Drivers/            # STM32 HAL 驱动
├── MDK-ARM/            # Keil 工程文件
├── foc/                # FOC 算法、控制和 PWM 板级适配代码
│   ├── Inc/
│   │   ├── pid.h           # PID 控制器
│   │   ├── foc_math.h      # FOC 数学函数 (Clarke/Park/InvPark)
│   │   ├── svpwm.h         # SVPWM 空间矢量调制
│   │   ├── open_loop_vf.h  # 开环 V/F 控制
│   │   └── foc_hw_pwm.h    # PWM 硬件接口层
│   └── Src/
│       ├── pid.c
│       ├── foc_math.c
│       ├── svpwm.c
│       ├── open_loop_vf.c
│       └── foc_hw_pwm.c
├── communication/      # 串口通信、调试协议和上位机数据输出
│   ├── Inc/
│   │   ├── comm_ringbuf.h       # 通用字节环形缓冲区
│   │   ├── comm_uart.h          # 通用 UART 抽象层
│   │   ├── comm_uart_stm32.h    # STM32 HAL UART DMA 适配层
│   │   ├── comm_vofa.h          # VOFA+ JustFloat 输出
│   │   ├── comm_cmd.h           # 文本命令解析
│   │   └── comm_scope.h         # 调试示波输出配置
│   └── Src/
├── Docs/               # 文档
└── README.md           # 本文件
```

本轮三相电流采样测试、ADC 触发点调整和 mode 0/mode 1 验证记录见 [`Docs/FOC_L3_4_三相电流采样验证记录.md`](Docs/FOC_L3_4_三相电流采样验证记录.md)。

`Tests/` 为本地主机侧辅助测试目录，不纳入版本管理。

## 开发计划

### Phase 1: PWM 硬件接口
- [x] PWM 硬件接口调研
- [x] PWM 硬件接口层实现 (foc_hw_pwm)
- [x] TIM1 初始化配置（中心对齐、6路互补输出）
- [x] 死区配置
- [x] SD 引脚控制（PA5, DRIVER_SD）
- [x] PWM 输出验证

### Phase 2: SVPWM 算法
- [x] SVPWM 基础算法实现（7段式，扇区查表）
- [x] 扇区判断与切换
- [x] 占空比计算
- [x] 过调制限幅

### Phase 3: FOC 数学与控制
- [x] Clarke/Park/InvPark 坐标变换
- [x] 角度归一化
- [x] 电压矢量限幅
- [x] PID 控制器（anti-windup）

### Phase 4: V/F 控制
- [x] V/F 曲线设计
- [x] 频率爬坡（加减速）
- [x] 电压限幅
- [x] 电角度累加

### Phase 5: 电流采样与保护
- [x] ADC 配置（三相电流 + 母线电压）
- [ ] 电流零点校准与实际电流验证
- [ ] 过流/过压/欠压保护
- [ ] 堵转保护

### Phase 6: 系统集成
- [ ] Hall/Encoder 接口
- [x] UART 调试接口（USART1 + DMA，3Mbps，VOFA+ JustFloat 输出）
- [x] 串口文本命令输入（start/stop/freq）
- [x] 开环 V/f 控制流程（ADC 注入中断驱动）
- [ ] 性能优化

## 串口调试

当前 USART1 用于调试通信，串口参数为 `3000000, 8N1`。

### VOFA+ 输出

VOFA+ 使用 JustFloat 协议，当前支持两种输出配置：

- `VOFA_MODE_NORMAL`：普通监控模式，最多 20 通道，默认 1kHz。
- `VOFA_MODE_SCOPE`：示波模式，当前 3 通道，随 16kHz 控制中断采样发送。该模式当前会导致 VOFA+ 卡死，暂不启用，后续改为分频输出或触发捕获。

普通监控模式当前输出通道为：

| 通道 | 含义 | 单位 |
|---|---|---|
| 0 | U 相电流 | A |
| 1 | V 相电流 | A |
| 2 | W 相电流 | A |
| 3 | U 相电流 ADC 电压 | V |
| 4 | V 相电流 ADC 电压 | V |
| 5 | W 相电流 ADC 电压 | V |
| 6 | U 相 BEMF / 相电压 | V |
| 7 | V 相 BEMF / 相电压 | V |
| 8 | W 相 BEMF / 相电压 | V |
| 9 | 母线电压 | V |
| 10 | U 相 BEMF ADC 电压 | V |
| 11 | V 相 BEMF ADC 电压 | V |
| 12 | W 相 BEMF ADC 电压 | V |
| 13 | VBUS ADC 电压 | V |
| 14 | 实时 VDDA | V |
| 15 | VREFINT 原始值 | count |
| 16 | V/f 当前频率 | Hz |
| 17 | V/f 输出电压 | V |
| 18 | PWM A 相比较值 | tick |
| 19 | PWM B 相比较值 | tick |

当前默认输出配置由 `Core/Src/main.c` 中的 `VOFA_OUTPUT_MODE` 控制。

应用层输出模板位于 `Core/Src/app_debug.c`：

- `app_debug_fill_vofa_normal()`：填充普通监控模式通道，后续可直接在此函数中增删变量，或改为传入外部数组。
- `app_debug_fill_vofa_scope_pwm()`：填充示波模式通道，当前为 `cmp_a/cmp_b/cmp_c`，后续可替换为三相电流、`Ud/Uq/theta` 等 PWM 同步变量。

### 文本命令输入

当前第一版输入命令使用文本行协议，以 `\r` 或 `\n` 结尾：

```text
start
stop
freq 17.0
```

- `start`：启动开环 V/f 控制。
- `stop`：停止开环 V/f 控制。
- `freq x`：设置目标频率，当前限制为 `0.0Hz` 到 `30.0Hz`。

命令解析在主循环中执行，串口接收回调只负责把数据写入 ringbuffer。

### 测试步骤

1. 测试普通监控模式

确认 `Core/Src/main.c` 中配置为：

```c
#define VOFA_OUTPUT_MODE       VOFA_MODE_NORMAL
#define VOFA_NORMAL_PERIOD_MS  1U
```

VOFA+ 选择 JustFloat，串口参数为 `3000000, 8N1`。连接后应看到 20 个通道，其中 0-9 为当前有效通道，10-19 为预留通道。

2. 测试文本命令输入

在普通监控模式下发送：

```text
freq 10
start
stop
```

观察通道变化：

| 命令 | 预期现象 |
|---|---|
| `freq 10` | 通道 1 变为 10 |
| `start` | 通道 4 从 STOP 进入 ALIGN/RAMP/RUN |
| `stop` | 通道 4 回到 STOP，通道 0 回到 0 |

3. 测试示波模式

当前保留该模式作为后续设计入口，暂不建议启用。实测 VOFA+ 在 3 通道 16kHz 连续 JustFloat 输出下会卡死，后续需要改为 `16kHz 同步采样 + 分频输出`，或改为 RAM 触发捕获后再慢速发送。

将 `Core/Src/main.c` 中配置改为：

```c
#define VOFA_OUTPUT_MODE  VOFA_MODE_SCOPE
```

重新编译烧录。该模式只输出 3 个通道：

| 通道 | 含义 |
|---|---|
| 0 | PWM A 相比较值 `cmp_a` |
| 1 | PWM B 相比较值 `cmp_b` |
| 2 | PWM C 相比较值 `cmp_c` |

该输出在 ADC 注入转换完成回调中调用，和 16kHz PWM 控制周期同步。

4. 示波模式下测试输入

示波模式下仍支持 `start`、`stop`、`freq x` 输入，但该模式下串口输出接近 3Mbps 上限，建议只偶尔发送命令，不要连续刷命令。

若高速模式下 VOFA+ 卡顿、丢帧或曲线异常，优先判断为串口链路或上位机吞吐不足。可临时降低输出频率或改回低速多通道模式验证控制逻辑。

## 快速开始

### 开发环境
- Keil MDK-ARM v5
- STM32CubeMX（用于引脚配置）
- ST-Link 调试器

### 编译步骤
1. 使用 Keil 打开 `MDK-ARM/Foc_svpwm_VF.uvprojx`
2. 编译项目
3. 烧录到 MCU

### 硬件连接
参考 `Docs/FOC_L3_1_PWM硬件接口调研.md` 中的引脚分配表。

## 参考资料

- STM32F4 参考手册（RM0090）— 高级定时器章节
- 野火无刷电机驱动板原理图
- FOC 相关论文和教程

## 更新记录

| 日期 | 版本 | 修改内容 |
|---|---|---|
| 2026-06-27 | v0.1.0 | 初始版本，完成项目框架和 PWM 硬件接口调研 |
| 2026-06-27 | v0.2.0 | 迁移 FOC 库代码：SVPWM、坐标变换、PID、V/F 控制 |
| 2026-06-28 | v0.3.0 | 开环 V/f 控制实现：ADC 注入中断驱动控制链路，SD 引脚修正为 PA5，USART1+DMA 初始化 |
| 2026-07-06 | v0.4.0 | 调整目录结构：FOC 集中到 `foc/`，新增 `communication/`，接入 USART1 DMA、VOFA+ JustFloat 和文本命令输入 |

## 当前问题

- **过流保护触发**：电机可驱动但加速过程中驱动板过流灯闪烁，怀疑 accel=10 Hz/s 过快导致滑差过大，待降低加速度验证
- **高速示波输出卡死**：`VOFA_MODE_SCOPE` 以 16kHz 连续输出 3 通道 JustFloat 时，VOFA+ 会卡死。该模式暂不启用，后续改为分频输出或触发捕获模式。
- **ADC 参考电压待确认**：控制板实际 3.3 V 供电约为 3.0 V，PF9 万用表读数与软件按 3.3 V 换算的 ADC_VBUS 存在差异。当前先保持原理图的 VBUS `×37` 比例，优先确认 MCU 实际 VDDA，再考虑 VREFINT 动态换算。详细过程见 [`Docs/FOC_L3_3_ADC测量问题与排查记录.md`](Docs/FOC_L3_3_ADC测量问题与排查记录.md)。
- **三相电流尚未完成实机验证**：PA3/PA4/PA6 的 ADC1 电流通道已接入 VOFA+。当前分支启动时会在驱动器关闭状态下自动采集 2048 组样本完成三相零点校准，但尚未接入过流保护和电流环。

## ADC 测量输出

当前 ADC3 已接入相电压（BEMF）和母线电压测量：

| 引脚 | ADC 通道 | VOFA+ 通道 | 含义 |
|---|---|---:|---|
| PF6 | ADC3_IN4 | 10 | U 相 BEMF / 相电压（V） |
| PF7 | ADC3_IN5 | 11 | V 相 BEMF / 相电压（V） |
| PF8 | ADC3_IN6 | 12 | W 相 BEMF / 相电压（V） |
| PF9 | ADC3_IN7 | 13 | 母线电压（V） |

为便于校准，通道 14~17 输出 ADC 引脚实际电压，分别对应 U/V/W 相和 VBUS。通道 17 是判断母线换算比例的关键：如果母线实际为 12.3 V，按原理图 `POWER/37 + 1.24`，PF9 理论上应约为 `1.572 V`。

通道 18 输出由内部 `VREFINT` 计算得到的实时 `VDDA`，通道 19 输出 `VREFINT` 原始 ADC 值。ADC 电压换算不再固定依赖 3.3 V，正式公式为：

```text
VDDA = VREFINT_CAL × VREFINT_CAL_V / VREFINT_ADC原始值
ADC电压 = ADC原始值 × VDDA / 4095
```

当前已经完成代码接入，但仍需实机确认通道 18 与 MCU 的 `VDDA/VDD` 万用表测量值一致。

换算依据配套原理图：

```text
相电压/BEMF = (ADC原始值 × 3.3 / 4095 - 1.24) × 37
母线电压   = (ADC原始值 × 3.3 / 4095 - 1.24) × 37
```

原理图标注的 VBUS 比例为 ×37。当前实测母线 12.3 V、PF9 ADC 电压约 1.77~1.78 V，与理论 ADC 电压 1.572 V 不一致。当前代码仍按原理图使用 ×37，需继续核对实际板级分压网络、PF9 引脚连接和 ADC 参考电压。

## 许可证

MIT License
