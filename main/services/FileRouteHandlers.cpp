#include "FileRouteHandlers.hpp"
#include "FS.h"
#include "SPIFFS.h"

static const char* FILE_TAG = "FILE_ROUTES";

// In FileRouteHandlers.cpp (near the top)
static fs::FS* global_mounted_fs = nullptr;

// --- Implementation of File Handlers ---

/**
 * @brief Handles file list requests.
 * Route: GET /list
 */
void handle_list_files(AsyncWebServerRequest *request) {
    FileManager& fm = FileManager::getInstance();
    std::vector<FileManager::FileInfo> files = fm.list_files_with_size();
    String json = "[";
    for (size_t i = 0; i < files.size(); i++) {
        json += "{\"name\":\"" + String(files[i].name.c_str()) + "\",\"size\":" + String(files[i].size) + "}";
        if (i < files.size() - 1) {
            json += ",";
        }
    }
    json += "]";
    request->send(200, "application/json", json);
}

/**
 * @brief Handles file download requests.
 * Route: GET /download?file=<filename>
 */
void handle_download(AsyncWebServerRequest *request) {
    if (!request->hasParam("file")) {
        return request->send(400, "text/plain", "Missing 'file' parameter");
    }
    String filename = request->getParam("file")->value();
    FileManager& fm = FileManager::getInstance();
    // The AsyncWebServer file APIs expect a path relative to the mounted FS
    // (e.g. "/myfile.bin"), not the VFS mount point ("/spiffs/myfile.bin").
    // Build the FS-relative path here.
    String fullPath = String("/") + filename; 

    FileManager::FileInfo info = fm.get_file_info(filename.c_str());

    if (!info.exists) {
        ESP_LOGW(FILE_TAG, "Download file not found: %s", filename.c_str());
        return request->send(404, "text/plain", "File not found");
    }

   ESP_LOGI(FILE_TAG, "Download started for: %s, size: %zu", filename.c_str(), info.size);
    
    // Let the AsyncFileResponse set download headers by using the download flag.
    AsyncFileResponse* response = new AsyncFileResponse(
        *global_mounted_fs,
        fullPath,
        "application/octet-stream",
        true // serve as attachment (adds Content-Disposition)
    );

    request->send(response);
}

/**
 * @brief Handles file deletion requests.
 * Route: POST /delete?file=<filename>
 */
void handle_delete(AsyncWebServerRequest *request) {
    if (!request->hasParam("file")) {
        return request->send(400, "text/plain", "Missing 'file' parameter");
    }
    String filename = request->getParam("file")->value();
    FileManager& fm = FileManager::getInstance();

    if (fm.delete_file(filename.c_str()) == ESP_OK) {
        ESP_LOGI(FILE_TAG, "File deleted: %s", filename.c_str());
        request->send(200, "text/plain", "File deleted");
    } else {
        request->send(500, "text/plain", "Failed to delete file");
    }
}

/**
 * @brief Upload handler callback for ESPAsyncWebServer.
 * Note: ESPAsyncWebServer handles multipart/form-data parsing for us!
 */
void FileRouteHandlers::handle_upload(AsyncWebServerRequest *request, const String& filename_from_form, size_t index, uint8_t *data, size_t len, bool final) {
    FileManager& fm = FileManager::getInstance();
    
    // Get filename from query parameter for reliability
    String filename = request->hasParam("filename", true) ? request->getParam("filename", true)->value() : filename_from_form;

    if (index == 0) {
        // First chunk, open file for writing
        fm.open_file_for_write(filename.c_str(), request->contentLength()); 
        ESP_LOGI(FILE_TAG, "Starting upload for: %s", filename.c_str());
    }

    if (fm.write_file_chunk((const char*)data, len) != ESP_OK) {
        // Error handling: if a write fails, delete the partially written file
        fm.delete_file(filename.c_str());
        request->send(500, "text/plain", "File write failed.");
        fm.close_file();
        return;
    }

    if (final) {
        // Last chunk
        fm.close_file();
        ESP_LOGI(FILE_TAG, "Upload complete for: %s", filename.c_str());
        request->send(200, "text/plain", "Upload complete");
    }
}

// --- Route Registration ---

void FileRouteHandlers::setup_file_routes(AsyncWebServer& server, fs::FS& fs) {
    global_mounted_fs = &fs; // Store reference to mounted FS

    server.on("/list", HTTP_GET, handle_list_files);
    server.on("/download", HTTP_GET, handle_download);
    server.on("/delete", HTTP_POST, handle_delete);

    // UPLOAD (POST /upload)
    server.on("/upload", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            // This empty handler is needed to start the upload process
            request->send(200); 
        }, 
        [](AsyncWebServerRequest *request, String formFilename, size_t index, uint8_t *data, size_t len, bool final) {
            // The actual file handling is done here
            FileRouteHandlers::handle_upload(request, formFilename, index, data, len, final);
        }
    );
}
