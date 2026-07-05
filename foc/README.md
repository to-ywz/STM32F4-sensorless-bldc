# FOC 模块

本目录集中存放 FOC 相关算法、控制状态机和当前 STM32 PWM 板级适配层。

## 目录

| 目录 | 内容 |
|---|---|
| `Inc/` | FOC 模块头文件 |
| `Src/` | FOC 模块源文件 |

## 当前边界

- `pid`、`foc_math`、`svpwm`、`open_loop_vf` 不依赖 STM32 HAL，应保持为可复用算法层。
- `foc_hw_pwm` 当前依赖 STM32 HAL，属于板级 PWM 适配层。
- 后续如要继续抽象硬件接口，可将 `foc_hw_pwm` 拆成统一接口和 `pwm_hw_stm32` 实现。

