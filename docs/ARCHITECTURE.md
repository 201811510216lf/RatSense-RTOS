# RatSense architecture

The system is split into an autonomous STM32 real-time controller, an ESP32-S3
network gateway, and a non-essential local backend. The dependency direction is
Application -> Service -> Driver -> BSP. The Host build substitutes deterministic
interfaces for every BSP operation.

The STM32 owns pressure sampling, thermal acquisition, the training state machine,
motor safety, local event storage, display refresh, and the monotonic event clock.
The ESP gateway owns Wi-Fi, MQTT/TLS, NTP mapping, and protocol forwarding. An ESP
reset or network outage must leave local training safe and able to continue.

The current Host path is:

pressure samples -> PressureProcessor -> TrainingController -> Motor28BYJ48
-> PelletIr -> LogStore and protocol frames -> gateway simulator.

Thermal samples use MlxSubpagePairer -> ThermalFramePool -> RqGprModel -> LCD/log
and telemetry. Each ownership transition is explicit in the pool API. The
single-writer LogStore models the StorageTask boundary; no motor operation waits on
storage I/O.

The binary UART protocol uses an explicit little-endian header, COBS encoding, a
zero delimiter, and CRC-32/IEEE. No C or C++ object memory is sent directly.

## Safety invariants

The motor is disabled on boot. Self-check must pass before a local session can arm
it. Remote commands request a session transition and cannot call a GPIO operation.
Feed success requires an IR event; a motor completion without IR becomes a fault
after a bounded timeout. Duplicate IR events are ignored by event ID.

## Hardware boundary

The STM32 and ESP source trees contain port contracts and vendor-API integration
points. Exact pinmux, ADC/DMA instances, cache regions, SDMMC wiring, LCD control
signals, and flash layout remain TBD until the named boards and modules are
available. These are documented as NEEDS_HARDWARE rather than guessed.

