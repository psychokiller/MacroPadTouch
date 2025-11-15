#pragma once

#include "ESPAsyncWebServer.h"
#include "esp_log.h"
#include "FileManager.hpp"
#include "NvsConfigManager.hpp" // Assuming this exists for context
#include "WifiManager.hpp"      // Assuming this exists for context

// Define the core port and task stack size
#define ASYNC_SERVER_PORT 80
#define SERVER_TASK_STACK_SIZE 8192
#define TAG_SERVER "ASYNC_SERVER"

// External C-style entry point
extern "C" void start_async_web_server();

/**
 * @brief Manages the ESPAsyncWebServer instance and its lifecycle.
 * Implements the Singleton pattern (or similar) to ensure a single running server.
 * Handles the background FreeRTOS task and delegates all request handling.
 */
class AsyncWebServerManager
{
public:
    static AsyncWebServerManager &getInstance();
    // FreeRTOS Task function
    static void server_task(void *pvParameter);

private:
    AsyncWebServer *server;
    // Singleton dependencies
    FileManager &file_manager;
    NvsConfigManager nvs_manager;
    WifiManager &wifi_manager;

    AsyncWebServerManager();
    ~AsyncWebServerManager();

    // Prevent copy and assignment
    AsyncWebServerManager(const AsyncWebServerManager &) = delete;
    AsyncWebServerManager &operator=(const AsyncWebServerManager &) = delete;

    // Core functionality
    void run_server_setup();
    void register_routes();
};

// --- Route Handler Declarations (to be implemented in separate files) ---
namespace FileRouteHandlers
{
    void setup_file_routes(AsyncWebServer &server, fs::FS& fs);
}

namespace ApStaConfigRouteHandlers
{
    void setup_ap_sta_config_routes(AsyncWebServer &server);
}