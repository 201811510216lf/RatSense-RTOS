# Progress

Updated 2026-11-11 (Asia/Shanghai).

## Completed in this workspace

- Project bootstrap, README, licensing, ignore rules, CMake host targets and
  continuation files: IMPLEMENTED.
- Protocol v1 COBS/CRC codec, parser, message IDs and stream error counters:
  IMPLEMENTED and HOST_TESTED with MSVC; evidence in out/verification/ctest_host_v1.txt.
- Pressure processing with ADC conversion, IIR/baseline, hysteresis, debounce,
  long-press and DMA-window latency: IMPLEMENTED and HOST_TESTED.
- 28BYJ-48 half/full-step non-blocking driver with timeout and coil release:
  IMPLEMENTED and HOST_TESTED.
- Configurable IR edge/pulse de-duplication and explicit pellet counters:
  IMPLEMENTED and HOST_TESTED.
- MLX subpage pairer, fixed two-slot thermal frame pool, HTU21D CRC:
  IMPLEMENTED and HOST_TESTED.
- Synthetic 10-feature RQ-GPR inference with parameter CRC and invalid-model
  rejection: IMPLEMENTED. Real source/model audit: BLOCKED_EXTERNAL.
- Sharp P3502 168x144 1bpp framebuffer, dirty rows, row command and PBM export:
  IMPLEMENTED and HOST_TESTED.
- CRC-protected single-writer log format with partial-tail recovery and replay:
  IMPLEMENTED and HOST_TESTED.
- Training FSM with safe boot, self-check, feed timeout and IR idempotency:
  IMPLEMENTED and HOST_TESTED.
- Host simulator, gateway simulator, UART tool, LCD preview and log decoder:
  IMPLEMENTED.
- ESP-IDF project and STM32 port contracts: DESIGNED/IMPLEMENTED, target compile
  and hardware behaviour are NEEDS_HARDWARE/BLOCKED_EXTERNAL.
- Local Mosquitto/FastAPI/SQLite/WebSocket backend: IMPLEMENTED; Python ingest/schema
  tests HOST_TESTED; Docker runtime is pending an available daemon.

## Environment blockers

- CMake and CTest were not on PATH, but usable copies were found under the Visual
  Studio/MATLAB installations. MSVC 19.44 built all Host targets and CTest passed.
  The exact generic command still requires those tools to be placed on PATH in a
  fresh environment.
- arm-none-eabi GCC, STM32CubeH7, ESP-IDF and physical boards are absent:
  TARGET_COMPILED and HIL_TESTED are pending.
- Docker daemon availability has not been established.
- Reference repository access failed with DNS/thread startup errors.

## Truthful status

No item in this file is marked HARDWARE_VERIFIED. Synthetic model output is not a
temperature accuracy claim, and simulator timings are not MCU timing claims.

