# Next actions

1. Expose the already discovered CMake/MSVC/CTest tools on PATH in a clean
   environment, then repeat the README commands if a portable build artifact is
   required. The current build and CTest evidence is already under out/verification.
2. Re-run every simulator scenario only after code changes; the current full
   scenario output is in out/verification/scenarios.txt.
3. Re-run the three reference repository audits with network access and lock exact
   commits/licenses.
4. Obtain the real RQ-GPR model and RAT-MONITOR source; export the ten feature
   order and golden vectors without claiming accuracy until validated.
5. Purchase/confirm boards, sensors, ULN2003, SD socket, power and instruments.
6. Generate CubeMX pinmux/linker/FreeRTOS sources, replace port-contract stubs and
   build the STM32 target.
7. Set the ESP-IDF version, module GPIOs and CA provisioning, then run idf.py build
   and UART/MQTT tests.
8. Run Docker Compose, send gateway simulator payloads, validate SQLite duplicate
   handling and WebSocket updates.
9. Perform safe-window OTA fault injection on Host, then separately instrument
   real flash erase/write/reboot/rollback.

