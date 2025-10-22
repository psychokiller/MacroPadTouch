idf_component_register(
    SRCS "WifiManager.cpp"
    INCLUDE_DIRS "."
    REQUIRES "esp_wifi" "nvs_flash" "esp_event" "NvsConfigManager"
)