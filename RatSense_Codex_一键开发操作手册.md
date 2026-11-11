# RatSense-RTOS：Codex 单文件自主开发操作手册（最终版）

> **用途**：将本 Markdown 文件放入 Codex 可访问的空工作目录，告诉 Codex：**“请完整阅读本文件，按其中的自主执行协议在当前工作区持续完成开发、构建、测试、修复和交付，不必每阶段请求我确认。”** 这一句话只需发送一次。文件本身也是全部任务说明。
>
> **目标**：面向嵌入式底层秋招，完成 **STM32H743 + FreeRTOS + 自研设备驱动 + ESP32-S3 通信网关 + UART DMA + MQTT + 本地数据持久化 + 简易 Web 监控 + 固件升级设计/可验证实现** 的开源工程。**没有开发板时先完成所有可在 Host 复现的软件、协议和集成测试；硬件相关部分编译/静态检查后明确保留待实测条目**。这份手册不承诺无硬件即可通过实机验收。
>
> **基线硬件**：STM32H743ZIT6 / NUCLEO-H743ZI2（待购）、ESP32-S3 开发板（待购）、薄膜 FSR 压力传感器 RP-C7.6-ST-LF2 + 现有电压/电阻转换模块、28BYJ-48 5V 五线四相减速电机、Adafruit P3502 Sharp Memory LCD 168×144、MLX90640 32×24、HTU21D、红外颗粒检测器、microSD。**ULN2003 驱动板、压力转换电路输出范围、红外模块类型、SD 卡座的 SPI/SDMMC 引脚及电源模块均待核实**。

---

## 0. 给 Codex 的最高优先级执行协议

你（Codex）是负责整套工程落地的资深嵌入式系统开发者；不要把本文件总结成计划就停止。**必须实际编辑、运行和验证仓库中的文件**。可独立决定内部实现细节及合理默认值，不需要用户逐阶段确认。工作以可复现的实际代码和测试为准。

**自主循环（全程执行）**：
1. 先扫描当前目录和已有文件；不要覆盖用户代码。若当前目录为空，初始化项目；若已有 RatSense 项目，做增量迭代。
2. 读取下面的全部约束，建立 `AGENTS.md`、`docs/PROGRESS.md`、`docs/VERIFICATION_MATRIX.md` 和 `docs/DECISIONS.md`。把计划分成可追踪的、具有测试标准的工作包。
3. 分析三个参考仓库及关键官方文档；若网络不可用或仓库不存在，记录失败，不得假装看过源码；继续完成不依赖远程资源的模块。禁止直接修改、覆盖参考仓库。
4. 自主按“基础架构 → 驱动/模拟器 → 完整业务闭环 → STM32/FreeRTOS → ESP32-S3 → UART 协议 → MQTT/后台 → OTA → 自动化联调”推进。**每阶段通过本阶段测试后自动进入下一阶段，不要向用户提问或等待批准**。
5. 每个工作包遵循 `设计 → 实现 → 编译 → 单测 → 集成测试 → 故障注入 → 文档更新 → 提交`。有 Git 时允许使用小提交，禁止破坏性清理、force push 或覆盖用户分支。
6. 遇到错误自己读构建日志并修复；不能以空类、常量返回、只打印日志的假函数或跳过测试来冒充功能。确实受限于硬件、外部依赖或凭据时，在进度矩阵标注 `BLOCKED_EXTERNAL`，继续其他工作。
7. **完成定义**：所有可在本机执行的构建与测试实际通过；STM32 和 ESP32 工程在有真实工具链/SDK 时完成交叉编译；无硬件部分明确标记 `NEEDS_HARDWARE`，不得写成 `HARDWARE_VERIFIED`。每个剩余问题必须给出准确复现步骤和下一次硬件联调方法。
8. 不得因为上下文长度或一次执行资源限制而谎称全部完成；把精确进度写入 `docs/PROGRESS.md`、`docs/NEXT_ACTIONS.md`，下一次 Codex 打开仓库可以从这里无缝续跑。当前可执行的工作应尽量完成。
9. 外部 SDK、依赖和样例需遵守许可证；禁止提交 Wi-Fi 密码、MQTT Token、私钥、设备证书。将机密使用 NVS、安全配置、环境变量或 `sdkconfig.defaults` 模板管理，`.gitignore` 排除私密配置。
10. 凡是芯片规格、引脚、DMA 可访问 SRAM、HAL API、ESP-IDF DMA/UART、LCD 时序、OTA Flash 布局有不确定性，先核对原厂文档和实际版本；**不要凭想象补齐细节**。将硬件连接图标为待实机确认。

### 0.1 状态标签（强制统一）
`DESIGNED` / `IMPLEMENTED` / `HOST_TESTED` / `TARGET_COMPILED` / `HIL_TESTED` / `HARDWARE_VERIFIED` / `BLOCKED_EXTERNAL`。

`HOST_TESTED ≠ TARGET_COMPILED ≠ HARDWARE_VERIFIED`；PC 上的虚拟时间统计不是 STM32 实时性能，模拟 OTA 成功不是物理 Flash 容错成功。

### 0.2 可自主解决和不得猜测的边界
- **可自主决定**：项目目录、组件内部实现、默认 MQTT Topic、测试数据组织、一般 CMake 选项、无特殊约束的任务栈初值、日志字段。
- **必须保留可配置/待核实**：电机真实每颗投食步数、28BYJ-48 具体线序、ULN2003 接线、实际压力分压网络与电压范围、红外检测电平/探测位置、MLX90640 型号/视场角、RTC 后备供电、Sharp LCD 的 DISP/EXTIN/EXTMOD 具体连接、SDMMC 引脚、电机电源电流、STM32 H743 的实际 Flash 分区适用性。
- 优先使用安全默认值：启动后**禁止自动投食**，只有显式进入训练模式且自检通过才能使能电机；硬件故障时禁止自动反复投食，需满足安全恢复条件。

---

## 1. 参考代码、代码风格、依赖锁定

必须实际尝试检查：
- **C++ 代码风格与工程组织参考**：https://github.com/JoeChen2me/RK-MediaProject
- **FED3 原开源项目与机构/显示设计**：https://github.com/KravitzLabDevices/FED3
- **原非接触体温项目指定分支**：https://github.com/201811510216lf/RAT-MONITOR-RELEASE/tree/codex/websocket-v2

步骤：查看每个仓库的 README、许可证、真实源文件、CMake、`include/`、`src/`、`.clang-format`（如果存在）。将**已经亲自读取的**代码风格特征、复用点、算法参数、限制和许可证写入 `docs/REFERENCE_AUDIT.md`。不能推测该项目的具体变量命名或架构。引用现有代码时，按其许可证处理，避免把原工程完整复制进来。

**语言**：C11 用于 CMSIS/HAL、FreeRTOS 桥接、启动和裸寄存器相关层；C++17 用于业务状态机、协议编解码、驱动协议层、Frame Pool、服务、模型与 Host 测试。ESP-IDF 可用 C 或 C++17，遵守所锁定版本 API。

C++ 风格目标：与 RK-MediaProject **实际观察到的**风格一致，保持 `include/` 与 `src/`、CMake 模块边界、RAII、明确 Buffer 所有权、错误处理、`std::unique_ptr` 等宿主端可用的所有权表达；STM32 固件不使用 `std::thread`/POSIX，不使用运行时无界分配；默认关闭 Target exceptions/RTTI（确有第三方依赖要求时记录例外）。禁止 ISR 中 new、malloc、阻塞、文件写入或调用普通 FreeRTOS API。

**版本管理**：用官方稳定版本 SDK，锁定确切版本号/commit 和 SHA（有条件时）。STM32CubeH7、CMSIS、FreeRTOS、ESP-IDF、ESP-MQTT、FatFs、MLX90640 参考实现、MQTT Broker、Host 测试框架分开管理；尽量使用官方 package manager/submodule/FetchContent 且可复现。不要把私有 SDK 链接写为公共下载依赖。

注意：新版本 ESP-IDF 中 ESP-MQTT 可能为独立组件。根据所锁定的 ESP-IDF 版本和组件管理说明，确定使用 `idf_component.yml` 的 `espressif/mqtt` 依赖还是 SDK 内置版本。**ESP32-S3 有 UART 驱动，ESP-IDF 默认 UART ring-buffer 驱动不等于用户直接配置了 UART DMA；必须查证 UHCI/GDMA 支持与 API，无法安全实现时退化为稳定的官方中断+ring-buffer UART，在 STM32 侧实现真实 UART DMA，并将 ESP 端 DMA 状态如实标注。**

官方参考：
- ST STM32H743 / NUCLEO-H743ZI2：https://www.st.com/en/evaluation-tools/nucleo-h743zi.html ；https://www.st.com/resource/en/user_manual/um2407-stm32h7-nucleo144-boards-stmicroelectronics.pdf
- STM32H743 参考手册 RM0433：ST 官方站查询最新对应修订。
- ESP-IDF ESP32-S3 UART：https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/uart.html
- ESP-MQTT：https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/protocols/mqtt.html
- Sharp P3502：https://www.adafruit.com/product/3502

---

## 2. 交付拓扑与双 MCU 职责

**STM32H743（实时主控）**：压力传感器 ADC/TIM/DMA、MLX90640/HTU21D I2C、28BYJ-48 四相驱动、红外计数、Sharp LCD SPI、SDMMC/FatFs、RTC、RQ-GPR MCU 推理、FreeRTOS、监控/低功耗、系统状态与本地安全策略。**断开网络/拔掉 ESP32 也能独立运行。**

**ESP32-S3（网络协处理器）**：通过 UART 与 STM32 双向通信；Wi-Fi STA、重连、DNS、NTP 时间同步（需可信时间来源）、TLS/MQTT 客户端、设备在线状态、遥测转发、受控远程配置、离线补传握手和 ESP OTA。绝不直接驱动电机或绕过 STM32 状态机。

**PC Host**：具有可重复的虚拟时钟、合成压力与热阵列数据、模拟电机/红外、1bpp 显示预览、文件存储、虚拟 UART 双端点，以及模拟 MQTT Broker/真实本地 Mosquitto 测试。

**可选但建议实现的本地后端**：Mosquitto + FastAPI/轻量 C++ 服务 + WebSocket 页面，至少提供在线状态、温度曲线、压力/计数、事件流与实验会话查询；后端不是 STM32 独立运行前提。可用 Docker Compose 本地启动，无公网服务依赖。

**关键流**：
1. 压力：FSR → ADC(TIM trigger) → DMA circular → ISR → PressureTask → TrainingFSM → MotorTask → 28BYJ-48 四相换相 → 光电验证 → 日志、显示、UART 事件发布。
2. 测温：MLX90640 I2C 原始数据 + HTU21D → 子页完整性/标定 → 温度矩阵 → 10 维特征（原模型一致）→ RQ-GPR C++ → 结果和质量位 → 显示/SD/UART。
3. 显示：单写入者 DisplayTask ← 最新快照 → 168×144 1bpp framebuffer → 脏行/局部刷新 → SPI → Sharp Memory LCD（独立且合规的 VCOM 翻转机制）。
4. 存储：关键事件与周期数据 → 有界双等级队列 → StorageTask 单写入者 → FatFs → SDMMC → microSD；存储出错不得阻塞电机控制。
5. 通信：STM32 UART DMA circular + IDLE/HT ISR → parser → command dispatcher → ACK/response；telemetry/events → TX bounded queue → UART DMA → ESP UART RX → MQTT/TLS；反向命令同链路返回。
6. 电源：外电源/电机独立驱动电源 + 传感器电源管理 → 监测/训练/暂停/故障/低功耗状态，保证 RTC 与可用唤醒源。

---

## 3. 硬件适配前的风险与必需文档

创建 `docs/HARDWARE_BOM.md`、`docs/POWER_DESIGN.md`、`docs/PINMUX_PLAN.md`、`docs/BENCH_BRINGUP.md`，按 `已有 / 待购 / 待核对` 分类。

- **FSR RP-C7.6-ST-LF2**：两引脚力敏电阻不是数字 ADC；测定原转换模块实际供电及模拟输出范围，若超过 STM32 ADC 允许范围必须重新设计 3.3V 分压/保护。绝不让 5V 模拟信号未经确认进入 ADC。力-阻标定具有非线性、迟滞、个体差异，不将 ADC 原始值宣传为准确牛顿力值。
- **28BYJ-48**：5V 五线单极四相，通常通过 ULN2003 或等效晶体管阵列驱动。**绝不能 GPIO 直驱绕组；也不是 A4988/DRV8825 的 STEP/DIR 接口**。4 根 GPIO 输出半步 8 拍/全步相序，由定时器中断安排短操作；线圈供电能力、实际减速比、步数/颗粒数需实测。配置电机异常时自动停止，防止夹伤/持续供料错误。
- **Sharp Adafruit P3502**：168×144、1bpp、约 3024 字节 framebuffer。不是 ST7789、没有 RGB565、不是需要背光的 TFT。核对具体面板控制协议、CS/SCLK/MOSI、VCOM/EXTMOD/EXTIN/DISP、行地址位序与电平转换，不使用错误时序。驱动必须考虑 VCOM 持续翻转要求。
- **MLX90640**：I2C、不同刷新率模式、交错子页、EEPROM 校准；不能用随机热图冒充实测体温；缺失真实训练参数时模型状态标记为测试模型。
- **HTU21D**：测量转换与 CRC。I2C 设备可共用总线但需要地址冲突、电平、总线带宽和上拉校核。
- **IR 颗粒传感器**：先留抽象和 active-high/low 参数，不知道具体光电器件就不能写死边沿含义；如需要分别统计投放和取走，预留第二红外通道。
- **microSD**：优先 SDMMC 4-bit 卡座，核查是否拥有 CLK/CMD/D0~D3 引脚与 3.3V 电平；若原卡座仅 SPI，则写独立 SPI 块驱动适配路径，不伪装成 4-bit SDMMC。
- **双 MCU 电平**：STM32 与 ESP32-S3 UART 接口确认 3.3V CMOS，TX→RX、RX←TX、GND 共地，建议另留 READY/RESET（可选）；避免接到 5V 串口。调试 UART 与 MCU 间数据 UART 分开。
- **供电**：电机 5V 需要独立足额电源/稳压与滤波，主控/ESP/Sensor 地共参考，防止电机电流冲击导致 Brownout；ESP Wi-Fi 峰值电流、电源线径、保险/反接/旁路一并核查。外接动物设备必须具备手动断电、人工供食替代和紧急停止方式。

**不凭空给 GPIO 具体编号**。让 CubeMX 对 NUCLEO-H743ZI2 做真实 pinmux 检查；ESP32-S3 避免芯片保留/启动/USB/Flash/PSRAM 冲突脚。输出 `pinmap.csv` 并注明板卡版号。未确认设备型号用 `TBD` 显示，不得悄悄选一个型号。

---

## 4. 代码工程组织（可调整但必须说明）

```text
RatSense-RTOS/
├─ AGENTS.md  README.md  LICENSE  .clang-format  .gitignore
├─ CMakeLists.txt  CMakePresets.json  cmake/  scripts/
├─ include/ratsense/{common,drivers,services,app,model,protocol}/
├─ src/{common,drivers,services,app,model,protocol}/
├─ platform/
│  ├─ host/{bsp,simulator}/
│  ├─ stm32h743/{Core,BSP,FreeRTOS,linker,startup,board_config}/
│  └─ esp32s3/{main,components,partitions.csv,sdkconfig.defaults,CMakeLists.txt}/
├─ server/{mosquitto,api,web,docker-compose.yml}/
├─ tools/{model_export,serial_sniffer,log_decoder,firmware_packager}/
├─ configs/{default,scenarios}/
├─ testdata/{thermal,pressure,uart,model,lcd}/
├─ tests/{unit,integration,protocol,fault,hardware_in_loop}/
├─ third_party/ (许可声明与锁定版本)
├─ .github/workflows/
└─ docs/{ARCHITECTURE.md,HARDWARE_BOM.md,POWER_DESIGN.md,PINMUX_PLAN.md,
         FREERTOS_DESIGN.md,MEMORY_DMA_CACHE.md,UART_PROTOCOL.md,MQTT_SCHEMA.md,
         TEST_MATRIX.md,REFERENCE_AUDIT.md,PORTING.md,PROGRESS.md,
         VERIFICATION_MATRIX.md,NEXT_ACTIONS.md,DECISIONS.md}
```

**两端独立构建**：Host/STM32 用 CMake 与 arm-none-eabi 工具链；ESP 使用 ESP-IDF `idf.py set-target esp32s3`, `idf.py build`；Server 用 Compose。协议层使用可共享的纯 C/C++ codec 或维护语言无关的二进制规范与 golden vectors，避免引入依赖冲突。

**依赖方向**：`Application → Service → Driver → BSP`，Host 与 Target 不可互相 include 私有硬件头；公共接口不应暴露 `HAL_HandleTypeDef`、`esp_mqtt_client_handle_t` 或 POSIX fd。独立定义 clock、GPIO、ADC、I2C、SPI、UART async、storage block、RTC、logger、bus-lock、power interfaces；明确失败返回、超时及 Buffer 生命周期。Host 用虚拟接口，STM32 用真实 HAL/LL，ESP 用真实 ESP-IDF。

---

## 5. 设备驱动模块的完成定义

### 5.1 Pressure Driver（C/C++）
TIM 硬件触发 ADC，DMA circular 双半区，ISR 只记录完成并通过 `...FromISR` 通知；PressureTask 消费完成半区。支持分压标定、raw 转实际电压、移动平均/IIR、中值滤波可选、开关迟滞、持续按压、去抖、长按、重复触发间隔、baseline 漂移、ADC 饱和/丢块。Host 提供噪声、阶跃、持续载荷、传感器断开测试。初期速率与 DMA 半区长度可配置，并计算**检测窗口引入的最坏额外延时**。

### 5.2 Motor28BYJ48 Driver
实现 8 拍半步序列、可选全步序列、方向、换相周期、运动步数、默认线圈释放、状态切换与安全超时。定时器用于安排非阻塞换相，GPIO 通过 ULN2003 控制相位，不能使用 STEP/DIR API 伪装。驱动只发电机完成/故障事件；投食成功必须通过 IR 检测确认。Host 验证 8 拍表、步数边界、连续指令、停止、故障、重复投食保护。

### 5.3 Pellet IR Driver
输入触发极性可配置，边沿时间戳、脉宽/防抖、超时与计数去重；区分 press_count、motor_attempt_count、pellet_pass_count、pellet_removed_count；缺少第二 IR 时 `pellet_removed_count` 标记未支持，不能以模拟数据伪装。

### 5.4 MLX90640 Driver
真实器件寄存器读取/配置、EEPROM、子页序号/时间戳、合法子页配对、温度恢复算法及质量位。参考原 RAT-MONITOR 现有实现并保留来源。固定预分配 raw frame 与 768 像素 float frame pool，FREE/FILLING/READY/PROCESSING/RELEASED 生命周期。每次推理先检查有效热场/遮挡/过期标志。Host 使用来源明确的原始测试向量，随机热场只能用于压力/调度模拟而不得用于精度结论。

### 5.5 HTU21D Driver
非阻塞测量命令、转换等待、CRC、多测量值融合和异常恢复，支持单独 I2C 总线或共享总线互斥。

### 5.6 RQ-GPR Model（MCU C++）
读指定分支源码验证 10 维特征次序、均值/标准差、RQ 核参数、支持点、alpha、输出反归一化；实现离线 Python 参数导出工具和目标只读 Flash 参数。C++ float32 推理、输入合法性、训练模型版本/CRC、PC numpy 与 MCU 结果一致性 golden test。若真实模型缺失，训练相关功能标记 `BLOCKED_EXTERNAL`，仅允许明确的 synthetic model test，不得生成假的体温准确率。

### 5.7 Sharp Memory LCD P3502
168x144 monochrome 1bpp framebuffer，文本/图元绘制，dirty lines，SPI 包封装、DMA TX、CS/VCOM 时序，EXTMOD/EXTIN/DISP 配置根据原厂资料确定，确保低刷新状态仍正确执行 VCOM 维护。Host 输出 PBM 图片并用 snapshot 测试，验证每一行地址顺序、位序、尾字节和 VCOM 翻转；不采用 RGB565/TFT 控制器驱动。

### 5.8 microSD + FatFs
Host 提供文件/可故障注入块设备，STM32 优先真实 SDMMC + FatFs。单独 `StorageTask`，有界分级事件队列、预分配 batch buffer、原子/可恢复记录格式，CRC+sequence+session_id+monotonic timestamp+RTC timestamp，区分采样与关键动作事件。明确 `f_sync` 策略、写失败、拔卡、重挂载、数据恢复边界、日志补传（UART/MQTT）。写入不应持锁阻塞 MotorTask。检查 SDMMC IDMA 与缓存地址范围，绝不统一使用错误的 DMA 初始化模板。

### 5.9 RTC / Power / Fault
RTC 初始化/可信时间标记、单调时间不受 NTP 跳变影响；看门狗只由通过健康检查的监控任务喂狗，HardFault 记录、错误码、上电状态恢复、不能在重启后自动重放危险电机命令。低功耗逐步支持 Idle/Tickless，注意持续测温、VCOM、UART 和唤醒源约束；没有实测电流不写功耗指标。

---

## 6. FreeRTOS 系统设计

任务建议：PressureTask、TrainingTask、MotorTask、ThermalTask、InferenceTask、DisplayTask、StorageTask、CommsTask、MonitorTask，可按资源合并。记录任务优先级、栈预算、Deadline、触发周期与最大阻塞等待；严禁平均分配优先级或让写 SD 的任务优先于安全关键控制。

- 静态创建任务/队列/Mutex/信号量优先：`xTaskCreateStatic`、`xQueueCreateStatic`、Task Notification 等。
- ISR 中只执行清中断、DMA index 和 `...FromISR` 通知，正确处理 `portYIELD_FROM_ISR`；检查 `configMAX_SYSCALL_INTERRUPT_PRIORITY`、NVIC 优先级、SysTick/PendSV/BASEPRI。禁止 ISR 调 FatFs、malloc、日志格式化和普通 Mutex。
- 串行设备单写入者或受控 Bus Manager；I2C bus mutex 不得跨等待电机或模型推理。
- 统一处理 queue full / frame pool exhausted / UART congestion，关键投食记录优先，显示帧可合并，永不静默丢失关键事件。
- 生产者消费者的 Frame Pool 必须声明所有权、读写权限和归还路径；必须有超时和引用泄漏测试。
- MCU 内存：DTCM、AXI SRAM、D2/D3 SRAM、ITCM、Flash 分区与链接脚本；DMA1/2 与 SDMMC IDMA 的总线可访问性独立核验。32B cache line 对齐、CPU→DMA Clean、DMA→CPU Invalidate 及必要的前置维护，避免缓存行共享污染；可使用 MPU 非缓存区。通过 map 文件/stack high-water 产出内存报告，所有实测性能只在目标硬件生成。

---

## 7. STM32 ↔ ESP32-S3 UART DMA 正式协议（必须完成）

### 7.1 物理接口
- STM32 UART TX → ESP RX；ESP TX → STM32 RX；共 GND，3.3V CMOS。可选 RTS/CTS，默认 8N1、921600 波特率作为候选**不是已验证值**，支持 115200/460800 fallback 与配置协商；不依赖串口打印文本作为正式协议。
- STM32 RX **DMA circular + UART IDLE**（若所锁定 HAL 支持 `HAL_UARTEx_ReceiveToIdle_DMA`，须核对实际回调语义），适当处理 DMA 半满/满和溢出；TX DMA 异步队列、完成事件、超时与串口恢复。STM32 使用专属 UART 与调试 UART 分离。
- ESP32-S3：优先 ESP-IDF 官方 UART buffered driver + event queue/ring buffer。**仅在官方文档和目标 SDK 确认且成功编译/实测后接 UHCI/GDMA UART**；绝不写未经确认的 `uart_dma_*` API。协议层与传输实现解耦。

### 7.2 帧格式（实现并文档化）
**推荐二进制、小端、COBS 封包 + 0x00 帧界定**。避免同时编写复杂的转义状态机与可变长 JSON 解析；若选择其他定帧方式（SLIP/length+sync），必须说明抗误码同步依据并有 fuzz test。

解码后帧头采用固定字段，**不得把 C 结构体内存直接发送**：

| 字段 | 位宽 | 描述 |
|---|---:|---|
| magic | 2B | 固定 0x5253（传输字节序由文档规定） |
| version | 1B | 协议版本 1 |
| msg_type | 1B | 消息类型 |
| flags | 1B | ACK_REQ / RESPONSE / ERROR 等 |
| header_len | 1B | 头长度，便于扩展 |
| sequence | 4B | 本端单调消息序列，可回绕 |
| ack_sequence | 4B | 应答的请求序列，无应答时 0 |
| payload_len | 2B | 上限强制检查 |
| device_time_ms | 8B | 单调时间，不与墙钟混淆 |
| payload | N | 明确定义的定长/TLV 数据，拒绝未知关键字段 |
| crc32 | 4B | 计算范围与多项式固定记录于规范 |

所有协议字节用 codec 显式读写，数值范围检查后再申请/使用 Buffer。COBS 后 0x00 分帧。设置 `MAX_PAYLOAD=512` 初值并可调，解析器必须限制最大帧长、长度异常、CRC 错误计数和过时版本行为。构建 golden binary vector、byte-by-byte split、合并包、随机噪声、丢字节、恶意长度和 fuzz 测试。CRC 对完整性检查有用但**不能充当身份认证**。

### 7.3 消息类型（两端完整实现）
- `HELLO` / `HELLO_ACK`：设备身份、硬件/固件/协议版本、feature bitmap、最大包长。
- `HEARTBEAT` / `STATUS`：在线、队列深度、故障码、存储状态、RSSI（ESP）、运行时统计。
- `THERMAL_SAMPLE`：体温预测值、温度质量、热阵列时间戳、环境温湿度；完整热图传输单独消息、限频并可压缩/降采样。
- `PRESS_EVENT`、`FEED_ATTEMPT`、`PELLET_DETECTED`、`ERROR_EVENT`：关键事件带 `event_id` 和实验 `session_id`，必须去重。
- `CONFIG_GET`、`CONFIG_SET`、`CONFIG_ACK/NACK`：范围验证、版本与鉴权、不可绕过本地安全约束。
- `START_SESSION`、`PAUSE_SESSION`、`RESUME_SESSION`、`STOP_SESSION`：状态机严格校验。远程指令不可直接执行电机动作；调试喂食须仅在安全维护模式显式启用。
- `TIME_SYNC`：墙钟映射，保留原 monotonic 时间，不允许 NTP 回拨影响事件顺序。
- `LOG_QUERY`、`LOG_RECORD`、`LOG_ACK`：STM32 本地记录补传，保证网络断线后有序/按序号可追踪地恢复。
- `FW_UPDATE_REQUEST` / `FW_UPDATE_STATUS`：后期 STM32 更新协商，受安全状态及签名验证约束。

### 7.4 可靠传输与背压
- 所有控制消息要求 `ACK/NACK`，重发超时和最大次数可配置。接收端以 `(sender_id, sequence, session_id)` 或命令 ID 实现幂等；重传不能导致重复投食。
- 遥测可合并、限频；关键事件优先、必须保留到被确认持久化/转发为止。UART ACK 仅表示**对端已接收/解析**，不能证明 MQTT Broker 已收到，更不能证明数据已持久化；定义分层 ACK：`UART_RECEIVED`、`BROKER_ACKED`、`BACKEND_STORED`（可选）。
- STM32 与 ESP 各自有有界队列与高低水位。拥塞时降低热图频率，禁止静默丢弃关键记录，错误统计输出。ESP 复位/断电时 STM32 正常运行。
- 会话建立处理两端重启、序列号回绕、时钟不同步、协议版本不匹配、异常长时间离线与重复包。

### 7.5 双端软件测试
`uart_protocol_test` 共用 golden vectors；`virtual_uart_test` 连接两端虚拟串口，注入丢包、乱序（从物理 UART 看顺序正确，模拟拆包/重传足够）、位翻转、CRC 错、ESP 重启、STM32 重启、拥塞与断连。通过后再进入真实双板连线。

---

## 8. ESP32-S3 网关固件（必须提供完整 ESP-IDF 工程）

建立 `platform/esp32s3` 独立工程，使用 ESP-IDF 官方组件，包含：
1. `WifiManager`：STA 连接、断线指数退避、漫游/重试、网络状态事件、IP 获取；SSID/密码通过非提交的配置/Provisioning 保存。
2. `UartLink`：官方 UART driver、RX 环形缓冲/事件队列、TX queue、协议 codec、统计、重连和双向命令。与 STM32 的实际 UART DMA 衔接；支持硬件流控能力探测。ESP 端 DMA 仅在真实支持且验证时启用。
3. `MqttClient`：ESP-MQTT，默认 `mqtts://` + CA 验证、稳定 Client ID、Last Will & Testament、Keepalive、QoS 0/1 对应策略、重连、发布确认与离线队列。禁止默认关闭 TLS 证书检查；本地开发可用独立 `DEV_INSECURE` 配置但不能在发行版启用。
4. `TelemetryBridge`：解码 STM32 数据、转换为版本化 MQTT payload、限频/合并热图、关键事件不丢弃、timestamp 与质量位保留。
5. `CommandBridge`：订阅指令 Topic、严格解析、认证/授权策略、参数范围检查、发 UART 命令、等待 ACK、向 MQTT 发布执行结果。MQTT 接收不能直接触发 GPIO 电机动作。
6. `OfflineReplay`：按需对接 STM32 microSD 补传；ESP 层队列可以短暂缓冲但不要把易失 RAM 当可靠存储。
7. `TimeManager`：联网后 SNTP/NTP 时间同步和可信时间状态，通过协议同步 MCU RTC；事件排序保持单调时间。
8. `HealthMonitor`：堆内存、Wi-Fi、UART 错包、MQTT 状态、丢消息、重连次数与看门狗。
9. `OtaManager`：ESP HTTPS OTA、镜像验证、回滚/boot confirmation、升级状态日志；禁止在设备训练/投食关键动作期间强制复位，需与 STM32 协商安全窗口。

设置 `sdkconfig.defaults`、独立 `partitions.csv`、组件版本和可复现构建说明。不得把 Wi-Fi 凭据写入仓库。相同协议编码/解码规则用于 ESP 与 Host，提供 ESP 编译测试/单元测试和网络状态机测试。

---

## 9. MQTT 规范与服务端联调（必须完整实现）

本地默认 Broker：Mosquitto（Docker Compose），不依赖外部公用 Broker。提供 TLS 自签开发环境说明和生产证书部署步骤；私钥不入库。推荐 MQTT 3.1.1 做兼容基础，若明确选择 MQTT5 必须锁定能力。

Topic 示例（`ratsense/v1/{device_id}`）：
- `telemetry/status`：online、firmware、uptime、fault、queues、storage。
- `telemetry/thermal`：timestamp、predicted_temp、quality、ambient、features/model_version。
- `telemetry/training`：pressure_summary、press_count、motor_attempt_count、pellet_count。
- `events`：event_id、session_id、type、sequence、timestamps、result。
- `cmd/request`：cmd_id、issued_at、expires_at、cmd_type、params、authorized_by（最小配置）。
- `cmd/response`：cmd_id、status、error_code、applied_version。
- `config/reported`：设备实际参数；`config/desired`：期望参数，版本冲突规则明确定义。
- `availability`：retained online/offline 与 Last Will，避免 retained 危险控制命令。

明确每个 Topic 的 JSON Schema 或 CBOR schema、保留消息设置、最大大小、发布频率、QoS 和持久化策略。QoS1 可重复投递，消费端必须按 event_id/cmd_id 去重；不可声称“QoS1=恰好一次”。为事件补传建立 `stored_ack` 协议，只有后端落盘后才可确认最终持久化。

Server：提供 `server/docker-compose.yml` 启动 Mosquitto + Python FastAPI（或用户参考风格的 C++ 后端，但以可维护为主）+ SQLite + 简单 Web 页。功能：设备在线状态、预测体温时间序列、按压与投食计数、事件实时 WebSocket、历史会话、CSV 导出、受授权的参数配置与命令响应。默认只绑定 localhost、登录或 Token、防跨站控制、输入校验，生产部署必须有鉴权。Web UI 不妨碍设备脱网自治。

集成测试：启动 Broker / 后端 / 虚拟 ESP，发送 UART Host 模拟帧 → MQTT → API/SQLite → WebSocket 消息 → 配置命令 → MQTT → ESP → UART → STM32 FSM 校验 → ACK。支持断网、Broker 重启、重复消息、消息顺序、Token 无效、后端落盘失败和恢复补传。

---

## 10. OTA / 双 MCU 固件升级（软件全部实现、实机安全验证待到货）

**ESP32-S3**：按所锁定 ESP-IDF 版本实现官方 `esp_https_ota` 方案，合理的 OTA_0/OTA_1 + otadata 分区，固件完整性/来源校验、反回滚策略（视芯片安全能力与部署策略）、启动确认/回滚、安全窗口检查、版本拒绝、网络中断恢复。编译验证时不得声明已实机回滚成功。

**STM32H743**：先核对所选 STM32H743ZIT6 的实际 Flash 容量、扇区布局与现有固件镜像大小。建立独立 Bootloader 工程、固件镜像头（magic、version、target_id、size、hash、signature、min_bootloader_version）、Flash 写入/验证抽象与升级状态机。使用**数字签名验证**固件可信来源（例如适合 MCU 的成熟密码库和验证算法），CRC 仅用于链路错误检测。STM32 固件由 ESP HTTPS 下载并通过 UART 分块传输至 STM32 的安全暂存区；必须验证完整镜像后才考虑切换。双槽位仅在 Flash 确认足够时采用，否则使用经确认的外部 QSPI/OSPI Flash 暂存或提供不能保证断电回滚的明确限制，绝不强行把两份固件塞入不足的 Flash。

强制条件：训练结束/暂停+电机停稳、日志已冲刷、供电稳定、固件版本兼容、签名验证成功后才能切换；禁止异常升级导致电机输出危险状态。必须有升级被中断、错签名、错误 target、版本回退、UART 中断、Flash 写失败、Boot confirm 超时的 Host 故障注入和 Target 编译测试。无硬件时 Bootloader 的 Flash 擦写/跳转/看门狗可靠性保持 `NEEDS_HARDWARE`。

---

## 11. Host 模拟器与集成测试是第一优先交付

创建下列运行程序/测试工具：
- `ratsense_sim`：可以跑多个实验会话，模拟压力曲线/热阵列/环境数据/电机励磁/IR/单色 LCD/文件存储；有确定性虚拟时间和故障注入。
- `ratsense_gateway_sim`：模拟 ESP-S3 UART/网络，具备 MQTT payload 构造与协议回环，**与实际 ESP-IDF 固件区分**。
- `uart_frame_tool`：串口 binary dump/分析/CRC 检查/golden vector。
- `model_export`：Python 导出模型，C++ 交叉验证。
- `log_decoder`：按时间戳和序列恢复事件，检查断点与重复记录。
- `lcd_preview`：输出 PBM/PNG 的 168x144 单色屏画面。

场景：`normal`, `pressure_bounce`, `long_press`, `motor_timeout`, `pellet_missing`, `mlx_i2c_nack`, `thermal_subpage_loss`, `model_invalid`, `sd_full`, `sd_removed`, `esp_reset`, `uart_crc_error`, `uart_overflow`, `mqtt_disconnect`, `mqtt_duplicate`, `broker_restart`, `command_replay`, `ota_bad_signature`, `power_loss`。使用虚拟时间进行长时测试，避免 CI 真的等待数天；实时真机长测另设硬件测试计划。

单测至少覆盖：FSR 阈值/滤波、28BYJ 相序、投食 FSM、IR 去重、HTU21D CRC、MLX 子页、Frame Pool 生命周期、GPR golden vector、LCD 位序/VCOM/脏行、记录 CRC/恢复、UART COBS/CRC/fuzz、状态机幂等、MQTT Topic Schema、重传与退避、ESP 离线期间本地投食继续、OTA 不安全状态拒绝。

使用 CTest、ASan/UBSan（Host）、clang-format、clang-tidy（有时）、静态检查；提供 GitHub Actions 的 Host 构建测试与可选 ARM/ESP 构建 jobs，工具链不可获得时给出明确 skip 说明。优先让工程在纯 Linux 环境中一条命令完成 Host 构建和所有不依赖硬件的测试。所有性能指标区分模拟结果与真实测量。

---

## 12. 完整的构建、启动、验收命令

Codex 必须最终在 README 中给出并实际验证可执行命令（如果命令行路径不同，README 与脚本保持一致；禁止仅写示例而未测试）：

```bash
# 1. Host 构建和测试
cmake -S . -B build/host -DRATSENSE_PLATFORM=HOST_SIM -DCMAKE_BUILD_TYPE=Debug
cmake --build build/host -j
ctest --test-dir build/host --output-on-failure

# 2. 单设备模拟及故障注入
./build/host/ratsense_sim --scenario normal
./build/host/ratsense_sim --scenario uart_crc_error
./build/host/ratsense_sim --scenario esp_reset

# 3. STM32 交叉编译：工具链和 STM32CubeH7 真正就绪时
cmake -S . -B build/stm32 -DRATSENSE_PLATFORM=STM32H743 -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake
cmake --build build/stm32 -j

# 4. ESP32-S3 真正 ESP-IDF 环境就绪时
cd platform/esp32s3
idf.py set-target esp32s3
idf.py build

# 5. 本地 Broker/API/网页联调（具备 Docker 时）
cd ../../server
docker compose up --build -d
# 使用自动化脚本从 Host UART 模拟端发数据，验证 MQTT -> backend -> SQLite/WebSocket
```

构建输出：Host 可执行程序、STM32 `.elf/.bin/.map`（能编译时）、ESP `.bin`（能编译时）、协议向量、测试报告、静态内存预算、版本化固件包（能构建时）。

**自动测试合格门槛**（在 Host 上）：所有关键单测通过；集成示例能按压→运动→光电确认→LCD→microSD 模拟文件→UART→MQTT→后端记录；断连后主控仍可离线训练；错误固件不得进入安装；关键事件在重复 UART/MQTT 报文中不可产生重复投食。

**实机验收另列**：示波/逻辑分析仪捕获 TIM、ADC/DMA、I2C、SPI VCOM、SDMMC、UART RX IDLE 和 ESP 真实通信；实测内存/Cache、采样误差、电机换相、投食计数、USB/SWD 烧录、Wi-Fi 断网重连、MQTT TLS、OTA 回滚、长时压力和物理故障注入。标注未完成项。

---

## 13. 自主开发阶段（内部按顺序，绝不每阶段停下来等用户）

**A. Project bootstrap**：参考仓库审计、AGENTS、CMake、SDK 版本锁定、CI、Host 最小可执行与测试。

**B. Training chain**：FSR 处理、28BYJ-48 四相驱动、红外事件、训练 FSM、Host 端闭环与故障测试。

**C. Thermal chain**：I2C、MLX90640 原始协议和子页、HTU21D、帧池、RQ-GPR 模型移植、golden tests。

**D. Display / Storage**：Sharp P3502 1bpp/VCOM、显示模拟器、SDMMC/FatFs 抽象、日志故障恢复、实验会话。

**E. STM32**：FreeRTOS Static API 与任务通知，NVIC/PendSV/SysTick/BASEPRI 设计说明，STM32CubeH7 BSP、TIM/ADC DMA、I2C、SPI DMA、SDMMC、UART TX/RX DMA、RTC、MPU/DCache；目标编译。

**F. ESP32**：ESP-IDF Wi-Fi、UART、协议、MQTT TLS、网络重连、设备状态、时间同步、离线补传；目标编译。

**G. Full bridge**：COBS 帧、CRC、序列号、ACK、幂等、拥塞、离线重放、指令响应；Host 跨进程测试与 Broker 联调。

**H. Backend**：本地 Mosquitto、API、SQLite、WebSocket、可视化和配置命令；Docker 启动与 e2e。

**I. OTA**：ESP 官方 HTTPS OTA 工程、STM32 安全 Bootloader/镜像格式、分块传输与升级状态机、双端仿真；实机保留验证门槛。

**J. Harden / deliver**：ASan/UBSan、错误注入、lint、完整 README、BOM、电路验证清单、用例日志、性能指标占位、HIL 测试指导、单页秋招项目技术架构图和简历可用的**真实性说明**。

**Codex 自动继续规则**：一旦某一阶段本机可执行测试通过，直接开始下一阶段；不需要询问用户。如遇不可解决的物理硬件缺失，标记 `NEEDS_HARDWARE` 并继续所有其余模块。不要每阶段输出大篇咨询话语；在 `docs/PROGRESS.md` 持续更新即可，最终一次交付汇总。

---

## 14. 最终验收清单（必须逐项打钩并附证据）

- [ ] Host CMake 编译 + CTest 全部通过，ASan/UBSan 通过。
- [ ] FSR 单次/连续按压与噪声测试通过，ADC 驱动的目标初始化/配置可编译。
- [ ] 28BYJ-48 八拍换相和非阻塞步数控制通过，真实机构参数有待校准标记。
- [ ] 红外确认、去重、投食超时/故障和无自动重复奖励通过。
- [ ] MLX90640 子页、校准、frame pool 与 C++ GPR 的 golden tests 通过；真实模型缺失时不得标记精度已验证。
- [ ] P3502 168x144 1bpp 预览、SPI 协议封装和 VCOM 管理通过软件测试。
- [ ] FatFs 与单写入者日志、故障恢复、离线记录与回放测试通过。
- [ ] FreeRTOS 任务、通知、静态队列、ISR 设计与 SRAM/DMA/Cache 分区得到源码和构建验证。
- [ ] STM32 RX Circular DMA + IDLE、TX DMA 的真实 BSP 代码完成并在 SDK 可用时目标编译。
- [ ] 二进制串口协议有 golden vectors、CRC/COBS、版本、序列号、重传、背压和 fuzz 测试。
- [ ] ESP32-S3 Wi-Fi、官方 UART 驱动、ESP-MQTT/TLS、断线重连、远程配置代码完整且可构建（有工具链时）。
- [ ] Mosquitto → Backend → SQLite → WebSocket/简单网页贯通，测试 MQTT 重复投递和重连。
- [ ] ESP HTTPS OTA 与 STM32 安全升级链路具有编译验证和故障注入；实机 Flash 回滚明确待测。
- [ ] README 的执行命令真实有效，BOM、引脚/电源风险、许可证、架构文档完整。
- [ ] 每个模块在 `VERIFICATION_MATRIX.md` 中有状态、测试记录、失败原因和实机验证步骤。
- [ ] 在总结中明确列出用户**仍需购置/确认**的硬件：STM32H743 板、ESP32-S3 板、ULN2003（若没有）、SDMMC 卡座（若原模块仅 SPI）、稳定电源/共地/适配、调试工具、所有传感器型号与电压。
- [ ] 秋招简历材料**只能把实际已完成/测试的部分写成已实现**，禁止杜撰 P99 延迟、体温 MAE、功耗、稳定运行小时数或 OTA 实测成功率。

---

## 15. 结束时 Codex 的最终回复格式

任务完成或受不可解外部依赖阻塞时，仅输出以下内容，并确保对应证据已写进仓库：

1. **实现概览**：列出已实现的软件组件、实际可运行命令和执行结果。
2. **构建/测试摘要**：Host、ARM、ESP-IDF、Server 各自 Passed/Failed/Skipped 及真实日志路径。
3. **剩余硬件验证**：按引脚、供电、DMA/Cache、LCD VCOM、电机换相、SDMMC、真实 UART/MQTT、OTA 分组；不要用模拟结果代替。
4. **项目文件位置**：关键代码、文档、测试、Docker Compose 和工具的具体路径。
5. **秋招技术亮点**：仅依据真实代码和测试结果，说明体现了哪种 STM32 驱动、FreeRTOS、内存、DMA、协议和可靠性能力。

**现在开始实际执行，直到完成全部不依赖物理开发板的可交付代码。不要停留于方案，也不要要求用户逐阶段发送新指令。**
