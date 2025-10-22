#include "NvsConfigManager.hpp"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#define TAG_NVS    "NVS_CONFIG"

NvsConfigManager* NvsConfigManager::nvs_manager_instance = nullptr;

NvsConfigManager::NvsConfigManager() {
    // 1. Initialize NVS (Non-Volatile Storage)
    // NOTE: This should typically be done once in app_main, but is left here for robustness
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Failed to initialize NVS flash: %s", esp_err_to_name(ret));
    }

    if (ret == ESP_OK) {
        ESP_LOGI(TAG_NVS, "NVS flash initialized successfully.");
    }
}

NvsConfigManager NvsConfigManager::getInstance() {
    if (nvs_manager_instance == nullptr) {
        nvs_manager_instance = new NvsConfigManager();
    }
    return *nvs_manager_instance;
}

// --- Private Implementation ---

/**
 * @brief Generic function to load credentials using specific NVS keys.
 */
esp_err_t NvsConfigManager::load_credentials(const char* ssid_key, const char* pass_key, 
                                           char *ssid, size_t ssid_len, char *pass, size_t pass_len) const {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_NVS, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size_ssid = ssid_len;
    size_t required_size_pass = pass_len;
    
    // Read SSID
    err = nvs_get_str(nvs_handle, ssid_key, ssid, &required_size_ssid);
    if (err != ESP_OK) {
        ESP_LOGW(TAG_NVS, "NVS key '%s' not found or error: %s", ssid_key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return ESP_ERR_NVS_NOT_FOUND;
    }
    
    // Read Password (can fail if not set, but we assume it is if SSID is set)
    nvs_get_str(nvs_handle, pass_key, pass, &required_size_pass);

    nvs_close(nvs_handle);
    ESP_LOGI(TAG_NVS, "Loaded credentials for key %s: %s", ssid_key, ssid);
    return ESP_OK;
}

/**
 * @brief Generic function to save credentials using specific NVS keys.
 */
esp_err_t NvsConfigManager::save_credentials(const char *ssid_key, const char *pass_key, 
                                           const char *ssid, const char *pass) const {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_NVS, "Error opening NVS handle for write: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, ssid_key, ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_NVS, "Failed to write SSID: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_set_str(nvs_handle, pass_key, pass);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_NVS, "Failed to write password: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_NVS, "Failed to commit NVS changes: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}


// --- Public API Implementation ---

esp_err_t NvsConfigManager::load_sta_credentials(char *ssid, size_t ssid_len, char *pass, size_t pass_len) const {
    return load_credentials(NVS_STA_SSID_KEY, NVS_STA_PASS_KEY, ssid, ssid_len, pass, pass_len);
}

esp_err_t NvsConfigManager::save_sta_credentials(const char *ssid, const char *pass) const {
    return save_credentials(NVS_STA_SSID_KEY, NVS_STA_PASS_KEY, ssid, pass);
}

esp_err_t NvsConfigManager::load_ap_credentials(char *ssid, size_t ssid_len, char *pass, size_t pass_len) const {
    return load_credentials(NVS_AP_SSID_KEY, NVS_AP_PASS_KEY, ssid, ssid_len, pass, pass_len);
}

esp_err_t NvsConfigManager::save_ap_credentials(const char *ssid, const char *pass) const {
    return save_credentials(NVS_AP_SSID_KEY, NVS_AP_PASS_KEY, ssid, pass);
}
