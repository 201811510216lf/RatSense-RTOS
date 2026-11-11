idf_component_register(
    SRCS "app_main.cpp" "../../../src/protocol/frame.cpp"
    INCLUDE_DIRS "." "../../../include"
    REQUIRES driver esp_wifi esp_event nvs_flash mqtt esp_http_client esp_https_ota
)

