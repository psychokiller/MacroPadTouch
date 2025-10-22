#pragma once
#include "NvsConfigManager.hpp"
#include "esp_err.h"
#include "esp_netif.h"
#include <vector>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "app_events.hpp"

struct WifiNetwork {
    std::string ssid;
    bool is_open; // True if the network is open (no password)
};

class WifiManager {
 public:
     // Get the singleton instance
     static WifiManager& getInstance();

     // Initializes the system, loads config, and starts APSTA mode
     esp_err_t start_apsta();

     // Function to get the latest scanned Wi-Fi networks
     std::vector<WifiNetwork> get_networks();

     // Status function
     bool is_sta_connected() const { return m_is_sta_connected; }

 private:
     WifiManager(NvsConfigManager* nvs_manager); // Private constructor
     ~WifiManager();

     static WifiManager* m_instance; // Static instance pointer
     NvsConfigManager* m_nvs_manager;
     esp_netif_t* m_sta_netif = nullptr;
     esp_netif_t* m_ap_netif = nullptr;

     // Members for managing STA connection logic and state
     std::vector<WifiNetwork> m_networks;
     SemaphoreHandle_t m_networks_mutex;
     std::string m_sta_ssid;
     std::string m_sta_password;
     bool m_has_saved_credentials = false;
     bool m_is_sta_connected = false;

     // Static event handler required by the C-API, delegates to the instance method
     static void s_event_handler(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data);

     // Instance method that processes the events
     void event_handler(esp_event_base_t event_base, int32_t event_id, void* event_data);

     // Private helper to configure and set the STA mode (only loads NVS data now)
     esp_err_t configure_sta();

     // Private helper to configure and set the AP mode
     esp_err_t configure_ap();

     // Performs a Wi-Fi scan
     void scan_networks(); 
     
     // New function: Checks scan results and attempts connection if saved network is found
     void check_and_connect_saved_network();

     // Static task function for periodic scanning
     static void scan_task(void* pvParameters); 
 };
