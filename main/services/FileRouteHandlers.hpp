#pragma once

#include "ESPAsyncWebServer.h"
#include "FileManager.hpp"
#include "esp_log.h"
#include <string>

namespace FileRouteHandlers {
    /**
     * @brief Sets up all file-related routes: Download, Upload, Delete, and List.
     */
    void setup_file_routes(AsyncWebServer &server, fs::FS& fs);

    // Internal helper for file upload
    void handle_upload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
}