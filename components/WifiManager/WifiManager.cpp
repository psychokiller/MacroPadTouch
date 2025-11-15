#include "WifiManager.hpp"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include <cstring>
#include <sstream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "WIFI_MANAGER";

#define AP_DEFAULT_SSID "MacroPadTouchAccessPoint"
#define AP_DEFAULT_PASS "Pas$W0rd_123@456+789"
#define MAX_WIFI_LEN 64
#define SCAN_INTERVAL_MS 15000   // Scan every X seconds
#define MAX_WIFI_SCAN_RESULTS 12 // Max number of APs to retrieve

// Initialize the custom event base
ESP_EVENT_DEFINE_BASE(APP_EVENTS);

// Static instance pointer
WifiManager *WifiManager::m_instance = nullptr;

WifiManager &WifiManager::getInstance()
{
    static NvsConfigManager nvs_manager_instance = NvsConfigManager::getInstance();
    if (m_instance == nullptr)
    {
        m_instance = new WifiManager(&nvs_manager_instance);
    }
    return *m_instance;
}

WifiManager::WifiManager(NvsConfigManager *nvs_manager) 
    : m_nvs_manager(nvs_manager), 
      m_networks_mutex(xSemaphoreCreateMutex()),
      m_has_saved_credentials(false), // Initialize new members
      m_is_sta_connected(false)       // Initialize new members
{
    // TCP/IP stack initialization
    ESP_ERROR_CHECK(esp_netif_init());

    // Register instance event handler
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &s_event_handler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &s_event_handler, this));

    // Wi-Fi initialization
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // --- Set Country Code for wider channel scan (1-13) ---
    wifi_country_t country = {
        .cc = "CA",
        .schan = 1,
        .nchan = 13,
        .policy = WIFI_COUNTRY_POLICY_AUTO,};
    ESP_ERROR_CHECK(esp_wifi_set_country(&country));

    // Create the scan task
    xTaskCreate(scan_task, "wifi_scan_task", 4096, this, 5, NULL);
}

WifiManager::~WifiManager()
{
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &s_event_handler);
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &s_event_handler);
    esp_wifi_deinit();
    esp_netif_deinit();
    vSemaphoreDelete(m_networks_mutex);
}

esp_err_t WifiManager::start_apsta()
{
    m_sta_netif = esp_netif_create_default_wifi_sta();
    m_ap_netif = esp_netif_create_default_wifi_ap();
   
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    configure_ap();
    configure_sta();

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi APSTA startup sequence complete. STA connection will be attempted after first scan.");
    return ESP_OK;
}

StaStatus WifiManager::get_sta_status() const {
    StaStatus status;
    // Lock the critical sections (optional, but good practice)
    if (xSemaphoreTake(m_networks_mutex, pdMS_TO_TICKS(100)) == pdPASS) {
        status.is_connected = m_is_sta_connected;
        // Only return a saved SSID if credentials have been set
        if (m_has_saved_credentials) {
            status.saved_ssid = m_sta_ssid;
        }
        status.ip_address = m_ip_address;
        xSemaphoreGive(m_networks_mutex);
    } else {
        ESP_LOGE(TAG, "Failed to get mutex for STA status.");
    }
    return status;
}

esp_err_t WifiManager::configure_sta()
{
    char sta_ssid[MAX_WIFI_LEN + 1] = {0};
    char sta_pass[MAX_WIFI_LEN + 1] = {0};

    esp_err_t err = m_nvs_manager->load_sta_credentials(sta_ssid, sizeof(sta_ssid), sta_pass, sizeof(sta_pass));

    if (err == ESP_OK && sta_ssid[0] != '\0')
    {
        m_sta_ssid = sta_ssid;
        m_sta_password = sta_pass;
        m_has_saved_credentials = true;
        ESP_LOGI(TAG, "STA credentials loaded for SSID: %s. Connection is deferred.", m_sta_ssid.c_str());
    }
    else
    {
        m_has_saved_credentials = false;
        ESP_LOGW(TAG, "No STA credentials found. STA interface remains passive.");
    }
    return err;
}

esp_err_t WifiManager::configure_ap()
{
    char ap_ssid[MAX_WIFI_LEN + 1] = {0};
    char ap_pass[MAX_WIFI_LEN + 1] = {0};

    if (m_nvs_manager->load_ap_credentials(ap_ssid, sizeof(ap_ssid), ap_pass, sizeof(ap_pass)) != ESP_OK || ap_ssid[0] == '\0')
    {
        strncpy(ap_ssid, AP_DEFAULT_SSID, sizeof(ap_ssid));
        strncpy(ap_pass, AP_DEFAULT_PASS, sizeof(ap_pass));
        ESP_LOGI(TAG, "Using default AP config: %s", AP_DEFAULT_SSID);
    }

    wifi_config_t ap_config = {};
    strncpy((char *)ap_config.ap.ssid, ap_ssid, sizeof(ap_config.ap.ssid));
    strncpy((char *)ap_config.ap.password, ap_pass, sizeof(ap_config.ap.password));
    ap_config.ap.ssid_len = strlen(ap_ssid);
    ap_config.ap.channel = 1;
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode = (strlen(ap_pass) < 8) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;

    ESP_LOGI(TAG, "AP configured as: %s", ap_ssid);
    return esp_wifi_set_config(WIFI_IF_AP, &ap_config);
}

void WifiManager::check_and_connect_saved_network()
{
    if (!m_has_saved_credentials || m_is_sta_connected) {
        return; 
    }

    bool network_found_in_scan = false;

    if (xSemaphoreTake(m_networks_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        for (const auto& network : m_networks)
        {
            if (network.ssid == m_sta_ssid)
            {
                network_found_in_scan = true;
                break;
            }
        }
        xSemaphoreGive(m_networks_mutex);
    }

    if (network_found_in_scan)
    {
        ESP_LOGI(TAG, "Saved network '%s' found in scan. Configuring and connecting...", m_sta_ssid.c_str());

        wifi_config_t sta_config = {};
        strncpy((char *)sta_config.sta.ssid, m_sta_ssid.c_str(), sizeof(sta_config.sta.ssid));
        strncpy((char *)sta_config.sta.password, m_sta_password.c_str(), sizeof(sta_config.sta.password));
        sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK; // Assuming WPA2 for saved networks

        esp_err_t err_config = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        if (err_config != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set STA config: %s", esp_err_to_name(err_config));
            return;
        }

        esp_err_t err_connect = esp_wifi_connect();
        if (err_connect == ESP_OK) {
             ESP_LOGI(TAG, "STA connect initiated successfully.");
        } else {
             ESP_LOGE(TAG, "STA connect failed immediately: %s", esp_err_to_name(err_connect));
        }
    }
    else
    {
        ESP_LOGW(TAG, "Saved network '%s' NOT found in scan. Skipping connection attempt.", m_sta_ssid.c_str());
    }
}


void WifiManager::scan_networks()
{
    ESP_LOGI(TAG, "Starting Wi-Fi scan...");

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0, // Set to 0 to scan all channels in the regulatory domain
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .coex_background_scan = true, // Enable coexistence background scan for stability
    };

    esp_err_t scan_err = esp_wifi_scan_start(&scan_config, true);
    if (scan_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Wi-Fi scan failed: %s", esp_err_to_name(scan_err));
        return;
    }
    ESP_LOGI(TAG, "Wi-Fi Scan Complete.");

    vTaskDelay(pdMS_TO_TICKS(10));

    wifi_ap_record_t *ap_records = new wifi_ap_record_t[MAX_WIFI_SCAN_RESULTS];
    uint16_t ap_count = MAX_WIFI_SCAN_RESULTS;

    esp_err_t get_records_err = esp_wifi_scan_get_ap_records(&ap_count, ap_records);

    if (get_records_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get AP records: %s", esp_err_to_name(get_records_err));
        delete[] ap_records;
        return;
    }

    ESP_LOGI(TAG, "Total APs found in scan: %u", ap_count); 

    if (xSemaphoreTake(m_networks_mutex, portMAX_DELAY) == pdTRUE)
    {
        m_networks.clear();
        for (int i = 0; i < ap_count; i++)
        {
            WifiNetwork network;
            network.ssid = (char *)ap_records[i].ssid;
            network.is_open = (ap_records[i].authmode == WIFI_AUTH_OPEN);
            m_networks.push_back(network);
            ESP_LOGD(TAG, "  -> %s (RSSI: %d)", network.ssid.c_str(), ap_records[i].rssi);
        }
        xSemaphoreGive(m_networks_mutex);
    }

    delete[] ap_records;

    check_and_connect_saved_network();
}

std::vector<WifiNetwork> WifiManager::get_networks()
{
    std::vector<WifiNetwork> networks;
    if (xSemaphoreTake(m_networks_mutex, portMAX_DELAY) == pdTRUE)
    {
        networks = m_networks;
        xSemaphoreGive(m_networks_mutex);
    }
    return networks;
}

void WifiManager::scan_task(void *pvParameters)
{
    WifiManager *wifi_manager = static_cast<WifiManager *>(pvParameters);
    while (true)
    {
        wifi_manager->scan_networks();
        vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
    }
}

// --- Event Handler Delegation ---

void WifiManager::s_event_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data)
{
    WifiManager *instance = static_cast<WifiManager *>(arg);
    instance->event_handler(event_base, event_id, event_data);
}

void WifiManager::event_handler(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_AP_START)
        {
            ESP_LOGI(TAG, "Wi-Fi SoftAP started.");
            esp_event_post(APP_EVENTS, APP_EVENT_WIFI_AP_STARTED, NULL, 0, portMAX_DELAY);
        }
        else if (event_id == WIFI_EVENT_STA_START)
        {
            ESP_LOGI(TAG, "Wi-Fi Station started. Deferring connect to scan task...");
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            m_is_sta_connected = false; 

            wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
            ESP_LOGE(TAG, "STA Disconnected! Reason: %d", event->reason); 

            ESP_LOGI(TAG, "STA disconnected. Retrying in 2s...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            
            if (m_has_saved_credentials) {
                esp_wifi_connect();
            }

            esp_event_post(APP_EVENTS, APP_EVENT_WIFI_DISCONNECTED, NULL, 0, portMAX_DELAY);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = static_cast<ip_event_got_ip_t *>(event_data);

        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        m_ip_address = ip_str;
        
        ESP_LOGI(TAG, "STA Got IP: %s", m_ip_address.c_str());

        m_is_sta_connected = true;
        
        esp_event_post(APP_EVENTS, APP_EVENT_WIFI_STA_CONNECTED, NULL, 0, portMAX_DELAY);
    }
}
