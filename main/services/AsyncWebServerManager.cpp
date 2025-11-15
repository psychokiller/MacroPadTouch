#include "AsyncWebServerManager.hpp"
#include "FileRouteHandlers.hpp"
#include "ApStaConfigRouteHandlers.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "FS.h"
#include "SPIFFS.h"

// Initialize the static instance
AsyncWebServerManager& AsyncWebServerManager::getInstance() {
    static AsyncWebServerManager instance;
    return instance;
}

AsyncWebServerManager::AsyncWebServerManager() 
    : server(new AsyncWebServer(ASYNC_SERVER_PORT)), file_manager(FileManager::getInstance()),
      nvs_manager(NvsConfigManager::getInstance()), wifi_manager(WifiManager::getInstance()) {
    // 1. Initialize SPIFFS before starting server
    if (file_manager.init_spiffs() != ESP_OK) {
        ESP_LOGE(TAG_SERVER, "Failed to initialize SPIFFS. Server not ");
        // Note: The server object will be deleted in the cleanup if the task fails.
    } else {
        ESP_LOGI(TAG_SERVER, "SPIFFS initialized successfully.");
    }
}

AsyncWebServerManager::~AsyncWebServerManager() {
    if (server) {
        server->end();
        delete server;
    }
}

void AsyncWebServerManager::run_server_setup() {
    // 2. Register all routing logic
    register_routes();
    
    // 3. Start the server
    server->begin();
    ESP_LOGI(TAG_SERVER, "Async Web Server started on port %d", ASYNC_SERVER_PORT);
}

void AsyncWebServerManager::register_routes() {
    ESP_LOGI(TAG_SERVER, "Registering routes...");

    fs::FS& mounted_fs = file_manager.get_mounted_fs();

    server->serveStatic("/", mounted_fs, "/web/").setDefaultFile("index.html"); 

    // 3. --- CLEAN URL HANDLERS FOR CONFIG PAGES ---
    server->on("/wifi/sta", HTTP_GET, [&mounted_fs](AsyncWebServerRequest *request) {
        request->send(mounted_fs, "/web/wifi_sta.html", "text/html");
    });
    server->on("/wifi/ap", HTTP_GET, [&mounted_fs](AsyncWebServerRequest *request) {
        request->send(mounted_fs, "/web/wifi_ap.html", "text/html");
    });

    server->on("/files", HTTP_GET, [&mounted_fs](AsyncWebServerRequest *request) {
        request->send(mounted_fs, "/web/file_manager.html", "text/html");
    });
 
    // File Management (Upload, Download, Delete)
    FileRouteHandlers::setup_file_routes(*server, mounted_fs);

    // Configuration (STA/AP WiFi setup)
    ApStaConfigRouteHandlers::setup_ap_sta_config_routes(*server);

    
    ESP_LOGI(TAG_SERVER, "Default file serving enabled from /spiffs/web/");

    // Optional: Handle 404 Not Found
    server->onNotFound([](AsyncWebServerRequest *request) {
        ESP_LOGW(TAG_SERVER, "404 Not Found: %s", request->url().c_str());
        request->send(404, "text/plain", "Not found");
    });
}

// --- FreeRTOS Task Management ---

void AsyncWebServerManager::server_task(void *pvParameter) {
    // Since ESPAsyncWebServer runs on the main loop of the current task, 
    // the task only needs to setup the server and then block indefinitely.
    
    AsyncWebServerManager* instance = (AsyncWebServerManager*)pvParameter;
    
    // ESPAsyncWebServer requires the loop to be running for server->begin() to work
    instance->run_server_setup();

    // Keep the task alive indefinitely for the server's event loop to run.
    // ESPAsyncWebServer doesn't block, so we block the task itself.
    vTaskDelay(portMAX_DELAY);
    
    // Cleanup (should only be reached if vTaskDelete is called elsewhere)
    ESP_LOGW(TAG_SERVER, "Server task shutting down.");
    delete instance;
    vTaskDelete(NULL);
}

extern "C" void start_async_web_server() {
    // This C function is the public entry point (like the original code)
    AsyncWebServerManager* instance = &AsyncWebServerManager::getInstance();

    if (xTaskCreate(
            AsyncWebServerManager::server_task,
            "async_web_server",
            SERVER_TASK_STACK_SIZE,
            instance,
            5,
            NULL) != pdPASS) {
        ESP_LOGE(TAG_SERVER, "Failed to create server task.");
    }
}