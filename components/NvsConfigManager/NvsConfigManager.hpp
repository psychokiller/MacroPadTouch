#ifndef NVS_CONFIG_MANAGER_HPP
#define NVS_CONFIG_MANAGER_HPP

#include "esp_err.h"
#include <cstddef> // for size_t

// NVS keys and namespace definitions (should be shared constants)
#define NVS_NAMESPACE "wifi_config"
#define NVS_STA_SSID_KEY "sta_ssid"
#define NVS_STA_PASS_KEY "sta_pass"
#define NVS_AP_SSID_KEY  "ap_ssid"
#define NVS_AP_PASS_KEY  "ap_pass"

/**
 * @brief Manages persistent Wi-Fi credentials using ESP-IDF Non-Volatile Storage (NVS).
 * Responsibility: Handles all low-level NVS read/write operations.
 */
class NvsConfigManager {
public:
    
    static NvsConfigManager getInstance();

    // --- Station (Client) Credentials ---
    esp_err_t load_sta_credentials(char *ssid, size_t ssid_len, char *pass, size_t pass_len) const;
    esp_err_t save_sta_credentials(const char *ssid, const char *pass) const;
    
    // --- Access Point (Host) Credentials ---
    esp_err_t load_ap_credentials(char *ssid, size_t ssid_len, char *pass, size_t pass_len) const;
    esp_err_t save_ap_credentials(const char *ssid, const char *pass) const;

private:
    NvsConfigManager();
    static NvsConfigManager *nvs_manager_instance;
    esp_err_t load_credentials(const char* ssid_key, const char* pass_key, 
                               char *ssid, size_t ssid_len, char *pass, size_t pass_len) const;
    esp_err_t save_credentials(const char *ssid_key, const char *pass_key, 
                               const char *ssid, const char *pass) const;
};

#endif // NVS_CONFIG_MANAGER_HPP
