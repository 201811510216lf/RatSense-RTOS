# STM32 to ESP32-S3 UART protocol v1

Physical link: 3.3 V CMOS, TX to RX, RX to TX, common ground, dedicated debug UART,
8N1. 921600 is a candidate; 115200 and 460800 are fallback values. The rate is not
hardware verified.

After COBS decoding, the frame is:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 2 | magic 0x5253, little endian |
| 2 | 1 | version 1 |
| 3 | 1 | message type |
| 4 | 1 | flags |
| 5 | 1 | header length, 24 |
| 6 | 4 | sender sequence |
| 10 | 4 | acknowledged sequence |
| 14 | 2 | payload length, maximum 512 |
| 16 | 8 | monotonic device time in milliseconds |
| 24 | N | typed payload |
| 24+N | 4 | CRC-32/IEEE over all preceding decoded bytes |

The encoded frame contains no zero bytes and is terminated by one 0x00 byte.
Codec code is in include/ratsense/protocol/frame.hpp and
src/protocol/frame.cpp. The parser accepts byte-by-byte or arbitrary DMA chunks,
rejects bad lengths/magic/version/CRC, and reports bounded error counters.

Message IDs are defined in the header for HELLO, STATUS, THERMAL_SAMPLE, training
events, session transitions, time sync, log replay and firmware update status.
Payloads are versioned byte arrays; unknown critical fields are rejected by the
dispatcher.

ACK semantics are layered: UART_RECEIVED means decoded by the peer,
BROKER_ACKED means MQTT accepted, and BACKEND_STORED means SQLite committed. QoS1
or retransmission never means exactly once. Event IDs and command IDs are the
idempotency keys. A duplicate command or IR event cannot create a second reward.

