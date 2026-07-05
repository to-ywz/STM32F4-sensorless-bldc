# 通信模块

本目录预留给串口通信、调试协议和上位机数据输出代码。

## 目录

| 目录 | 内容 |
|---|---|
| `Inc/` | 通信模块头文件 |
| `Src/` | 通信模块源文件 |

## 当前边界

- `Core/Src/usart.c` 和 `Core/Inc/usart.h` 仍由 CubeMX 生成，负责 USART1 外设初始化。
- 本目录用于放置手写通信逻辑，例如 VOFA+ 数据帧、串口命令解析、调试变量输出。
- 通信模块可依赖 `Core` 中的 `huart1`，但尽量不要把业务控制逻辑塞回 CubeMX 生成文件。

## 当前方案

- TX 使用 ringbuffer + DMA，调用者只写入通用通信接口，不直接持有 DMA 发送缓冲区。
- RX 使用 DMA + IDLE，接收数据先进入 ringbuffer，再通知解析层。
- VOFA+ 使用 JustFloat：多个 `float` 小端二进制数据后追加帧尾 `00 00 80 7F`。
- `comm_uart` 是通用通信抽象层，不包含 STM32 HAL 类型。
- `comm_uart_stm32` 是 STM32 HAL 适配层，负责把 `UART_HandleTypeDef`、DMA 和 HAL 回调接到通用接口上。
- `comm_ringbuf` 是通用字节环形缓冲区，不依赖 STM32。
- `comm_scope` 是调试示波输出模块，只负责输出模式、发送周期和 JustFloat 发送，不依赖 FOC 或 STM32 HAL。
- 暂不使用动态内存。后续若需要更强封闭性，再把 `comm_uart` 收敛为 opaque handle + 用户静态 storage。

## 当前调试输出

`Core/Src/main.c` 已接入 USART1 调试输出，当前通过 `SCOPE_MODE` 选择输出配置。

### 低速多通道模式

```c
#define SCOPE_MODE  SCOPE_MODE_REALTIME_LOW_RATE
```

该模式在主循环中按 `SCOPE_LOW_RATE_PERIOD_MS` 周期发送，当前默认 1ms，最多 20 通道：

| 通道 | 含义 | 单位 |
|---|---|---|
| 0 | V/f 当前频率 `vf.freq` | Hz |
| 1 | V/f 目标频率 `vf.target_freq` | Hz |
| 2 | V/f 输出电压 `vf.v_out` | V |
| 3 | V/f 电角度 `vf.theta_e` | rad |
| 4 | V/f 状态枚举 | - |
| 5 | 最近一次命令状态 | - |
| 6 | PWM A 相比较值 `cmp_a` | tick |
| 7 | PWM B 相比较值 `cmp_b` | tick |
| 8 | PWM C 相比较值 `cmp_c` | tick |
| 9 | SVPWM 扇区 | - |
| 10-19 | 预留 | - |

20 通道 JustFloat 每帧 84 字节，1kHz 输出约 840kbps，适合 3Mbps 串口长期观察。

### 高速 3 通道模式

```c
#define SCOPE_MODE  SCOPE_MODE_HIGH_RATE_3CH
```

当前该模式仅作为后续设计入口，暂不建议启用。实测 VOFA+ 在 3 通道 16kHz 连续 JustFloat 输出下会卡死，后续需要改为分频输出或触发捕获。

该模式在 ADC 注入转换完成回调中调用，和 16kHz PWM 控制周期同步。
当前输出三相 PWM 比较值：

| 通道 | 含义 | 单位 |
|---|---|---|
| 0 | PWM A 相比较值 `cmp_a` | tick |
| 1 | PWM B 相比较值 `cmp_b` | tick |
| 2 | PWM C 相比较值 `cmp_c` | tick |

3 通道 JustFloat 每帧 16 字节，16kHz 输出约 2.56Mbps，已经接近 3Mbps 串口的实用上限。

VOFA+ 配置为 JustFloat，串口参数为 3000000、8N1。

USART1 位于 APB2，总线时钟当前为 84MHz。3Mbps 在该时钟下可得到有效分频，
框架侧由 DMA 承担搬运。
实际使用时还需要确认 USB 转串口芯片、上位机驱动和接线质量支持 3Mbps。

## 当前输入命令

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
当前没有发送文本 ACK，避免和 VOFA+ JustFloat 输出混在同一个串口里；命令是否生效可通过 VOFA 通道 1、4、5 观察。

## ringbuffer 并发约束

- 当前 ringbuffer 面向单生产者、单消费者模型，例如串口接收事件写入、主循环读取。
- `head` 和 `tail` 使用 `volatile`，避免主循环和中断之间被编译器缓存旧值。
- `reset()` 会同时修改 `head` 和 `tail`，只能在无并发访问时调用；若中断或 DMA 回调可能访问，需要由调用者先进入临界区。
- 通用 ringbuffer 不直接关中断，临界区应放在 STM32 适配层或业务调用点处理。

## 关于乒乓缓冲

当前使用 TX ringbuffer 承接连续输出。它和乒乓缓冲的思想相近：业务层继续写入缓冲区，DMA 从缓冲区取数据发送。

固定双缓冲/乒乓缓冲适合固定长度、固定周期数据流：

```text
buffer A 正在 DMA 发送
buffer B 填充下一帧
DMA 完成后 A/B 交换
```

串口命令和 VOFA+ 帧长度可能变化，ringbuffer 更自然。后续如果发现 ringbuffer 拷贝开销不可接受，再为固定周期波形输出单独做 A/B 双缓冲。
