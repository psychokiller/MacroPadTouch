idf_component_register(
    SRCS "FileManager.hpp"
    INCLUDE_DIRS "."
    REQUIRES "esp_wifi" "nvs_flash" "esp_event" "NvsConfigManager"
)