# Host and target test matrix

| Area | Host test | Target/HIL test |
|---|---|---|
| Pressure | threshold, hysteresis, bounce, long press, DMA latency | ADC trigger/DMA waveform and analog range |
| Motor | eight half-step sequence, stop, timeout | ULN2003 phase voltage/current and real feed count |
| IR | polarity, pulse width, de-duplication | optical alignment, active level and pellet geometry |
| Thermal | subpage pairing, frame pool, model validity | MLX calibration and temperature accuracy |
| LCD | 1bpp bit order, dirty rows, VCOM command | SPI timing, DISP/EXTMOD/EXTIN/VCOM |
| Storage | CRC records, partial-tail recovery, full/removed fault | SDMMC IDMA, card removal, sync latency |
| UART | COBS/CRC, split chunks, fuzz/error counters | IDLE/DMA and electrical link at candidate baud |
| MQTT/backend | duplicate events, schema, SQLite and WebSocket | TLS, Wi-Fi reconnect and broker outage |
| OTA | safe-window and bad-signature policy | flash erase/write, reboot, rollback and power loss |

