# FreeRTOS design

The planned static tasks are MotorTask (priority 6), PressureTask (6),
TrainingTask (5), ThermalTask (4), CommsTask (4), DisplayTask (3), StorageTask
(3), and MonitorTask (7). These are budgets, not measured timing claims.

Tasks use static creation, bounded queues, task notifications, and explicit
timeouts. Motor safety and pressure deadlines outrank display and SD writes.
StorageTask is the sole writer for the log stream. Display frames may be merged;
critical events may not be silently dropped.

ISR code only clears peripheral flags, records DMA indices, and uses FromISR
notification APIs. It never calls FatFs, malloc, formatting, ordinary mutex APIs,
or the motor state machine. NVIC priorities and
configMAX_SYSCALL_INTERRUPT_PRIORITY must be checked in the generated FreeRTOS
port before target compilation.

The repository contains a C port contract in platform/stm32h743/FreeRTOS. It is
DESIGNED and IMPLEMENTED as a reviewable interface, but not TARGET_COMPILED
without STM32CubeH7 and the FreeRTOS kernel.

