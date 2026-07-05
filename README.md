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
│   │   └── comm_cmd.h           # 文本命令解析
│   └── Src/
├── Docs/               # 文档
└── README.md           # 本文件
```

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
- [ ] ADC 配置（三相电流 + 母线电压）
- [ ] 电流计算与校准
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

VOFA+ 使用 JustFloat 协议，当前输出通道为：

| 通道 | 含义 | 单位 |
|---|---|---|
| 0 | V/f 当前频率 `vf.freq` | Hz |
| 1 | V/f 目标频率 `vf.target_freq` | Hz |
| 2 | V/f 输出电压 `vf.v_out` | V |
| 3 | V/f 电角度 `vf.theta_e` | rad |
| 4 | V/f 状态枚举 | - |
| 5 | 最近一次命令状态 | - |

当前输出周期由 `Core/Src/main.c` 中的 `DEBUG_VOFA_PERIOD_MS` 控制。

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

## 许可证

MIT License
