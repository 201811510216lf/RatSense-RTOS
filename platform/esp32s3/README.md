# ESP32-S3 gateway

This is an ESP-IDF project. It uses the official UART buffered driver and event
queue, the shared COBS/CRC frame codec, ESP-MQTT, and the ESP-IDF HTTPS OTA
component. No unverified UHCI/GDMA UART API is used; the ESP side is explicitly
marked buffered-driver until a target board and SDK build prove DMA behaviour.

Create a local sdkconfig with Wi-Fi credentials and a CA certificate before
connecting to a broker. The checked-in defaults never contain credentials. UART
TX/RX pin routing is left as UART_PIN_NO_CHANGE until the exact module is known.

The partitions.csv file provides OTA_0/OTA_1 and an explicit data partition. The
target must still be tested for image size, flash geometry, TLS certificate
validation, rollback/boot-confirmation, and safe-window coordination with STM32.

