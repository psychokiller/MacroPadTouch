#pragma once

#include "esp_http_server.h"
#include "esp_err.h"
#include <cstddef>
#include <cstdint>
#include "FileManager.hpp"
#include "NvsConfigManager.hpp"

#define TAG_SERVER "HTTP_SERVER"
#define FILE_SERVER_TASK_STACK_SIZE 8192
#define DOWNLOAD_BUFFER_SIZE 1024
#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 64

extern "C" void start_http_file_server();

class HttpFileServer
{
public:
    HttpFileServer();
    ~HttpFileServer() = default;

    void run();

    static void server_task(void *pvParameter);

    // --- HTTP Request Handlers (MUST be PUBLIC for C-Wrappers to access) ---
    esp_err_t handle_root_get(httpd_req_t *req);
    esp_err_t handle_download_get(httpd_req_t *req);
    esp_err_t handle_upload_post(httpd_req_t *req);
    // This is the function the C-wrapper tried to access privately:
    esp_err_t handle_delete_post(httpd_req_t *req);
    esp_err_t handle_wifi_sta_config(httpd_req_t *req);
    esp_err_t handle_wifi_ap_config(httpd_req_t *req);

private:
    httpd_handle_t server_handle;
    const static NvsConfigManager nvs_manager;
    /**
     * @brief Decodes a URL-encoded string, handling both '+' and %XX encoding.
     */
    std::string url_decode(const std::string& encoded_str);

    // Utility functions remain private
    bool parse_post_param(const char *body, size_t body_len, const char *key, char *value, size_t value_len);
    esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);

    // Disallow copying and assignment
    HttpFileServer(const HttpFileServer &) = delete;
    HttpFileServer &operator=(const HttpFileServer &) = delete;
};
