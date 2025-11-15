#include "ApStaConfigRouteHandlers.hpp"
#include "NvsConfigManager.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "WifiManager.hpp"
#include "FileManager.hpp"
static const char *CONFIG_TAG = "CONFIG_ROUTES";

/**
 * @brief Parses JSON from the request body. (Simplified implementation)
 * In a real-world scenario, you'd use ArduinoJson for robust parsing.
 */
bool get_json_param(AsyncWebServerRequest *request, const char *key, String &value)
{
    // This is a minimal, non-robust example for demonstration.
    // Real-world: Use JSON library like ArduinoJson.
    if (request->hasParam(key, true))
    {
        value = request->getParam(key, true)->value();
        return true;
    }
    return false;
}

// --- Handler Implementations ---

/**
 * @brief Handles STA (Client) WiFi configuration.
 * GET: Returns current STA config (SSID, but masked password).
 * POST: Saves new STA config and reboots.
 */
void handle_sta_config(AsyncWebServerRequest *request)
{
    NvsConfigManager nvs_manager = NvsConfigManager::getInstance();

    if (request->method() == HTTP_GET)
    {
        char ssid_buf[33] = {0};
        char pass_buf[65] = {0};
        nvs_manager.load_sta_credentials(ssid_buf, sizeof(ssid_buf), pass_buf, sizeof(pass_buf));

        // Return JSON response (masking password)
        String json_response = "{\"ssid\":\"" + String(ssid_buf) + "\", \"password\":\"**MASKED**\"}";
        request->send(200, "application/json", json_response);
        return;
    }

    if (request->method() == HTTP_POST)
    {
        // Read JSON POST body (assuming simple body for demonstration)
        String new_ssid_str, new_pass_str;
        bool success = false;

        if (get_json_param(request, "ssid", new_ssid_str))
        {
            get_json_param(request, "password", new_pass_str); // Password is optional

            if (nvs_manager.save_sta_credentials(new_ssid_str.c_str(), new_pass_str.c_str()) == ESP_OK)
            {
                success = true;
                request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"STA credentials saved. Rebooting...\"}");
            }
            else
            {
                request->send(500, "application/json", "{\"status\":\"error\", \"message\":\"Failed to save credentials\"}");
            }
        }
        else
        {
            request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Missing SSID in request\"}");
        }

        if (success)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }
        return;
    }

    request->send(405, "text/plain", "Method Not Allowed");
}

// /**
//  * @brief Handles AP (Access Point) WiFi configuration.
//  * GET: Returns current AP config (SSID, but masked password).
//  * POST: Saves new AP config and reboots.
//  */
// void handle_ap_config(AsyncWebServerRequest *request) {
//     NvsConfigManager nvs_manager = NvsConfigManager::getInstance();

//     if (request->method() == HTTP_GET) {
//         char ssid_buf[33] = {0};
//         char pass_buf[65] = {0};
//         nvs_manager.load_ap_credentials(ssid_buf, sizeof(ssid_buf), pass_buf, sizeof(pass_buf));

//         String json_response = "{\"ssid\":\"" + String(ssid_buf) + "\", \"password\":\"**MASKED**\"}";
//         request->send(200, "application/json", json_response);
//         return;
//     }

//     if (request->method() == HTTP_POST) {
//         String new_ssid_str, new_pass_str;
//         bool success = false;

//         if (get_json_param(request, "ssid", new_ssid_str) && get_json_param(request, "password", new_pass_str)) {

//             if (new_pass_str.length() < 8 && new_pass_str.length() > 0) {
//                  request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"AP Password must be at least 8 characters\"}");
//                  return;
//             }

//             if (nvs_manager.save_ap_credentials(new_ssid_str.c_str(), new_pass_str.c_str()) == ESP_OK) {
//                 success = true;
//                 request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"AP credentials saved. Rebooting...\"}");
//             } else {
//                 request->send(500, "application/json", "{\"status\":\"error\", \"message\":\"Failed to save AP credentials\"}");
//             }
//         } else {
//             request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Missing SSID or Password in request\"}");
//         }

//         if (success) {
//             vTaskDelay(pdMS_TO_TICKS(1000));
//             esp_restart();
//         }
//         return;
//     }

//     request->send(405, "text/plain", "Method Not Allowed");
// }

/**
 * @brief **NEW HANDLER:** Provides a combined JSON response of network scan and STA status.
 * Route: GET /api/wifi/scan
 */
void handle_wifi_scan(AsyncWebServerRequest *request)
{
    WifiManager &wifi_manager = WifiManager::getInstance();

    // 1. Get networks
    std::vector<WifiNetwork> networks = wifi_manager.get_networks();

    // 2. Get STA status
    StaStatus status = wifi_manager.get_sta_status();

    // 3. Build the JSON response string
    String json_response = "{";

    // A. Add STA status object
    json_response += "\"sta_status\":{";
    json_response += "\"is_connected\":" + String(status.is_connected ? "true" : "false") + ",";
    json_response += "\"saved_ssid\":\"" + String(status.saved_ssid.c_str()) + "\",";
    json_response += "\"ip_address\":\"" + String(status.ip_address.c_str()) + "\"";
    json_response += "},";

    // B. Add Networks array
    json_response += "\"networks\":[";
    for (size_t i = 0; i < networks.size(); ++i)
    {
        json_response += "{";

        // **IMPORTANT:** Escape any backslashes or quotes in the SSID if necessary.
        // Also, an empty string ("") is used for hidden networks, which the JS handles.
        json_response += "\"ssid\":\"" + String(networks[i].ssid.c_str()) + "\",";

        json_response += "\"is_open\":" + String(networks[i].is_open ? "true" : "false");
        // You can add more fields here, e.g., rssi if you added it to WifiNetwork struct

        json_response += "}";
        if (i < networks.size() - 1)
        {
            json_response += ",";
        }
    }
    json_response += "]";

    json_response += "}";

    request->send(200, "application/json", json_response);
}

/**
 * @brief Handles AP (Access Point) WiFi configuration API.
 * GET: Returns current AP config (SSID, masked password).
 * POST: Saves new AP config via NvsConfigManager.
 */
void handle_ap_config(AsyncWebServerRequest *request) {
    NvsConfigManager nvs_manager = NvsConfigManager::getInstance();

    if (request->method() == HTTP_GET) {
        char ssid_buf[33] = {0};
        char pass_buf[65] = {0};
        
        esp_err_t err = nvs_manager.load_ap_credentials(ssid_buf, sizeof(ssid_buf), pass_buf, sizeof(pass_buf));
        
        String current_ssid = (err == ESP_OK) ? ssid_buf : "";
        String pass_mask = (current_ssid.length() > 0 && strlen(pass_buf) > 0) ? "********" : "";
        
        String json_response = "{";
        json_response += "\"ssid\":\"" + current_ssid + "\",";
        json_response += "\"password_mask\":\"" + pass_mask + "\"";
        json_response += "}";
        
        request->send(200, "application/json", json_response);
        return;

    } else if (request->method() == HTTP_POST) {
        String new_ssid;
        String new_password;

        if (get_json_param(request, "ssid", new_ssid) && get_json_param(request, "password", new_password)) {
            
            // Password must be 8-63 characters for WPA2, or empty for an open network.
            if (new_password.length() > 0 && (new_password.length() < 8 || new_password.length() > 63)) {
                request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Password must be between 8 and 63 characters, or empty for open network.\"}");
                return;
            }
            
            esp_err_t err = nvs_manager.save_ap_credentials(new_ssid.c_str(), new_password.c_str());
            
            if (err == ESP_OK) {
                request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"AP configuration saved. Reboot or reconfiguration required.\"}");
                return;
            } else {
                request->send(500, "application/json", "{\"status\":\"error\", \"message\":\"Failed to save AP config to NVS.\" }");
                return;
            }
        } else {
            request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Missing 'ssid' or 'password' parameter.\" }");
            return;
        }
    }
}

// --- Route Registration ---

void ApStaConfigRouteHandlers::setup_ap_sta_config_routes(AsyncWebServer &server)
{
    // STA Config (GET and POST to the same URI)
    server.on("/api/wifi/sta", HTTP_ANY, handle_sta_config);

    // **REGISTER NEW AP JSON API ROUTE**
    server.on("/api/wifi/ap", HTTP_ANY, handle_ap_config);

    // Wi-Fi Scan (GET)
    server.on("/api/wifi/scan", HTTP_GET, handle_wifi_scan);

    ESP_LOGI(CONFIG_TAG, "API config routes registered at /api/wifi/...");
}