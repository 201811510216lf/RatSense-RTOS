# Third-party dependency ledger

No external source tree is vendored in this initial workspace.

Planned, separately pinned dependencies:

- STM32CubeH7, CMSIS and FreeRTOS: exact release/commit must be recorded after
  the board/CubeMX project is obtained.
- ESP-IDF and ESP-MQTT component: exact IDF release must be selected before
  target compilation.
- FatFs and the MLX90640 reference implementation: license and commit must be
  audited against the actual source branch.
- Mosquitto, FastAPI and paho-mqtt are container/Python dependencies declared in
  server/requirements.txt and server/docker-compose.yml.

The reference repository network audit failed in this environment, so no source
or license is represented as inspected. This is BLOCKED_EXTERNAL.

