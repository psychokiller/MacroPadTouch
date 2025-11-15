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

struct StaStatus {
    bool is_connected = false;
    std::string saved_ssid = "";
    std::string ip_address = "0.0.0.0";
};

class WifiManager {
 public:
     static WifiManager& getInstance();

     esp_err_t start_apsta();

     std::vector<WifiNetwork> get_networks();

     StaStatus get_sta_status() const;

     bool is_sta_connected() const { return m_is_sta_connected; }

 private:
     WifiManager(NvsConfigManager* nvs_manager);
     ~WifiManager();

     static WifiManager* m_instance;
     NvsConfigManager* m_nvs_manager;
     esp_netif_t* m_sta_netif = nullptr;
     esp_netif_t* m_ap_netif = nullptr;

     std::vector<WifiNetwork> m_networks;
     SemaphoreHandle_t m_networks_mutex;
     std::string m_sta_ssid;
     std::string m_sta_password;
     bool m_has_saved_credentials = false;
     bool m_is_sta_connected = false;
     std::string m_ip_address;

     static void s_event_handler(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data);

     void event_handler(esp_event_base_t event_base, int32_t event_id, void* event_data);

     esp_err_t configure_sta();

     esp_err_t configure_ap();

     void scan_networks(); 

     void check_and_connect_saved_network();


     static void scan_task(void* pvParameters); 
 };
