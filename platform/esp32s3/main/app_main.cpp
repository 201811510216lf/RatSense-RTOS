#include "ratsense/protocol/frame.hpp"

#ifdef ESP_PLATFORM
#include "driver/uart.h"
#include "esp_event.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#endif

#include <cstdint>
#include <cstring>
#include <vector>

namespace {
constexpr char kTag[] = "ratsense_gateway";
constexpr int kUartPort = 1;
constexpr int kUartRxBuffer = 4096;
constexpr int kUartTxBuffer = 4096;

#ifdef ESP_PLATFORM
EventGroupHandle_t wifi_events = nullptr;
constexpr EventBits_t kWifiConnected = BIT0;

void wifi_event(void*, esp_event_base_t base, int32_t id, void*) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(wifi_events, kWifiConnected);
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_events, kWifiConnected);
    }
}

void init_wifi() {
    wifi_events = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event, nullptr));
    wifi_config_t station{};
#ifdef CONFIG_RATSENSE_WIFI_SSID
    std::strncpy(reinterpret_cast<char*>(station.sta.ssid), CONFIG_RATSENSE_WIFI_SSID, sizeof(station.sta.ssid));
#endif
#ifdef CONFIG_RATSENSE_WIFI_PASSWORD
    std::strncpy(reinterpret_cast<char*>(station.sta.password), CONFIG_RATSENSE_WIFI_PASSWORD,
                 sizeof(station.sta.password));
#endif
    station.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &station));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void init_uart() {
    uart_config_t config{};
    config.baud_rate = 115200;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;
    ESP_ERROR_CHECK(uart_driver_install(static_cast<uart_port_t>(kUartPort), kUartRxBuffer, kUartTxBuffer, 20,
                                        nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(static_cast<uart_port_t>(kUartPort), &config));
    // TX/RX pins remain UART_PIN_NO_CHANGE until the actual ESP32-S3 module is selected.
    ESP_ERROR_CHECK(uart_set_pin(static_cast<uart_port_t>(kUartPort), UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void mqtt_event(void*, esp_event_base_t, int32_t, void*) {}

void init_mqtt() {
    esp_mqtt_client_config_t config{};
    config.broker.address.uri = "mqtts://localhost:8883";
    config.credentials.client_id = "ratsense-esp32s3-template";
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&config);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event, nullptr));
    // CA certificate provisioning is mandatory in a deployed build; this template
    // intentionally refuses to silently disable TLS verification.
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
}

void uart_task(void*) {
    std::vector<std::uint8_t> rx(512U);
    ratsense::protocol::StreamParser parser;
    while (true) {
        const int count = uart_read_bytes(static_cast<uart_port_t>(kUartPort), rx.data(), rx.size(), pdMS_TO_TICKS(50));
        if (count > 0) {
            parser.feed(rx.data(), static_cast<std::size_t>(count), [](const ratsense::protocol::Frame& frame) {
                ESP_LOGI(kTag, "received %s seq=%lu", ratsense::protocol::message_type_name(frame.message_type),
                         static_cast<unsigned long>(frame.sequence));
            });
        }
    }
}
#endif
}  // namespace

extern "C" void app_main(void) {
#ifdef ESP_PLATFORM
    esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES || nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_result);
    init_uart();
    init_wifi();
    init_mqtt();
    xTaskCreatePinnedToCore(uart_task, "uart_link", 4096, nullptr, 8, nullptr, 1);
#else
    // Allows syntax-only tooling to inspect the project outside ESP-IDF.
#endif
}

