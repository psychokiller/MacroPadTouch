#include "HttpFileServer.hpp"
#include "web/HtmlGenerator.hpp"
#include "FileManager.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cstdio>

// --- Static Helpers ---

const NvsConfigManager HttpFileServer::nvs_manager = NvsConfigManager::getInstance();

/**
 * @brief Decodes a URL-encoded string, handling both '+' (space) and %XX (percent) encoding.
 * * This function iterates through the input string and performs the necessary
 * decoding operations. It handles the specific requirement of converting
 * percent-encoded hex values back to their ASCII characters.
 * * @param encoded_str The input URL-encoded string.
 * @return The decoded string.
 */
std::string HttpFileServer::url_decode(const std::string& encoded_str) {
    std::string decoded_str;
    size_t length = encoded_str.length();

    for (size_t i = 0; i < length; ++i) {
        char c = encoded_str[i];

        if (c == '+') {
            // Standard form-urlencoded convention: '+' represents a space
            decoded_str += ' ';
        } else if (c == '%') {
            // Found a percent-encoded sequence (%XX)
            if (i + 2 < length) {
                // Extract the two hex digits (e.g., '40' from '%40')
                char hex[3] = {encoded_str[i+1], encoded_str[i+2], '\0'};
                int value;
                
                // Use sscanf to safely convert hex string to integer
                if (sscanf(hex, "%x", &value) == 1) {
                    decoded_str += (char)value;
                    i += 2; // Skip the two hex digits
                } else {
                    // Invalid hex sequence, append '%' literally
                    decoded_str += c;
                }
            } else {
                // Incomplete sequence at end of string
                decoded_str += c;
            }
        } else {
            // Append all other characters literally
            decoded_str += c;
        }
    }
    return decoded_str;
}

// --- Updated Parameter Parsing Method ---

/**
 * @brief Utility function to parse a specific parameter from a request body (x-www-form-urlencoded).
 * The extracted value is URL-decoded before being copied to the output buffer.
 */
bool HttpFileServer::parse_post_param(const char *body, size_t body_len, const char *key, char *value, size_t value_len)
{
    // 1. Find the Key-Value Pair
    std::string body_str(body, body_len);
    std::string key_str = std::string(key) + "=";
    size_t key_pos = body_str.find(key_str);

    if (key_pos == std::string::npos)
    {
        return false;
    }

    size_t start_pos = key_pos + key_str.length();
    size_t end_pos = body_str.find('&', start_pos);

    // 2. Extract the Encoded Value String
    std::string encoded_val_str = body_str.substr(start_pos, end_pos == std::string::npos ? end_pos : end_pos - start_pos);

    // 3. Decode the Value
    std::string decoded_val_str = url_decode(encoded_val_str);

    // 4. Validate Length and Copy to Output Buffer
    if (decoded_val_str.length() >= value_len)
    {
        ESP_LOGE(TAG_SERVER, "Parsed and decoded value for key '%s' too long (%zu) for buffer (%zu).", 
                 key, decoded_val_str.length(), value_len);
        return false;
    }

    strncpy(value, decoded_val_str.c_str(), value_len - 1);
    value[value_len - 1] = '\0';
    return true;
}

/**
 * @brief Sets HTTP response content type based on file extension.
 */
esp_err_t HttpFileServer::set_content_type_from_file(httpd_req_t *req, const char *filename)
{
#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

    if (IS_FILE_EXT(filename, ".html") || IS_FILE_EXT(filename, ".htm"))
    {
        return httpd_resp_set_type(req, "text/html");
    }
    else if (IS_FILE_EXT(filename, ".jpeg") || IS_FILE_EXT(filename, ".jpg"))
    {
        return httpd_resp_set_type(req, "image/jpeg");
    }
    else if (IS_FILE_EXT(filename, ".png"))
    {
        return httpd_resp_set_type(req, "image/png");
    }
    else if (IS_FILE_EXT(filename, ".css"))
    {
        return httpd_resp_set_type(req, "text/css");
    }
    else if (IS_FILE_EXT(filename, ".js"))
    {
        return httpd_resp_set_type(req, "application/javascript");
    }
    // Default to binary stream for unknown types
    return httpd_resp_set_type(req, "application/octet-stream");
#undef IS_FILE_EXT
}

// --- Handler Delegation Methods (Private Class Logic) ---

HttpFileServer::HttpFileServer() : server_handle(nullptr) {}

esp_err_t HttpFileServer::handle_root_get(httpd_req_t *req)
{
    FileManager *fm = (FileManager *)req->user_ctx;

    // Delegate HTML generation
    std::string response_str = HtmlGenerator::generate_root_page(fm);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, response_str.c_str(), response_str.length());

    return ESP_OK;
}

esp_err_t HttpFileServer::handle_download_get(httpd_req_t *req)
{
    const char *filename_ptr = req->uri;
    const char *filename = filename_ptr + 1; // Skip the leading '/'

    if (strlen(filename) == 0)
    {
        return handle_root_get(req); // Route '/' to root handler
    }

    FileManager *fm = (FileManager *)req->user_ctx;

    // 1. Check if file exists and get its size
    FileManager::FileInfo info = fm->get_file_info(filename);

    if (!info.exists)
    {
        ESP_LOGW(TAG_SERVER, "GET File not found: %s", filename);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found on SPIFFS");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG_SERVER, "Starting file download for: %s, size: %zu", filename, info.size);

    // 2. Set headers
    set_content_type_from_file(req, filename);
    char content_len_str[16];
    snprintf(content_len_str, sizeof(content_len_str), "%zu", info.size);
    httpd_resp_set_hdr(req, "Content-Length", content_len_str);
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"");

    // 3. Open file for reading
    if (fm->open_file_for_read(filename) != ESP_OK)
    {
        ESP_LOGE(TAG_SERVER, "Failed to open file for reading: %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file");
        return ESP_FAIL;
    }

    // 4. Stream data
    char buf[DOWNLOAD_BUFFER_SIZE];
    int read_bytes;
    esp_err_t err = ESP_OK;
    size_t remaining_size = info.size;

    while (remaining_size > 0)
    {
        size_t bytes_to_read = std::min((size_t)DOWNLOAD_BUFFER_SIZE, remaining_size);

        read_bytes = fm->read_file_chunk(buf, bytes_to_read);

        if (read_bytes > 0)
        {
            if (httpd_resp_send_chunk(req, buf, read_bytes) != ESP_OK)
            {
                ESP_LOGE(TAG_SERVER, "Download send failed. Aborting.");
                err = ESP_FAIL;
                break;
            }
            remaining_size -= read_bytes;
        }
        else
        {
            break; // EOF or error
        }
    }

    fm->close_file();

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG_SERVER, "File '%s' download complete.", filename);
        httpd_resp_send_chunk(req, NULL, 0); // Send zero-length chunk to signal end
    }
    else
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File transfer error.");
    }

    return err;
}

esp_err_t HttpFileServer::handle_upload_post(httpd_req_t *req)
{
    const char *uri_prefix = "/upload/";
    const char *filename_start_ptr = req->uri + strlen(uri_prefix);

    ESP_LOGI(TAG_SERVER, "POST handler reached for URI: %s", req->uri);

    if (strlen(filename_start_ptr) == 0)
    {
        ESP_LOGE(TAG_SERVER, "No filename specified in URI: %s", req->uri);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing filename in URI.");
        return ESP_FAIL;
    }

    std::string filename_start(filename_start_ptr);
    FileManager *fm = (FileManager *)req->user_ctx;

    ESP_LOGI(TAG_SERVER, "Starting file upload for: %s, total body size: %d", filename_start.c_str(), req->content_len);

    if (fm->open_file_for_write(filename_start.c_str(), req->content_len) != ESP_OK)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file on SPIFFS");
        return ESP_FAIL;
    }

    esp_err_t err = ESP_OK;
    char skip_buffer[1024];
    int received_bytes = 0;

    // --- 1. Skip Initial Boundary and Part Headers ---
    int skip_len = httpd_req_recv(req, skip_buffer, sizeof(skip_buffer));

    if (skip_len <= 0)
    {
        ESP_LOGE(TAG_SERVER, "Initial read failed or empty body. %d", skip_len);
        err = ESP_FAIL;
    }
    else
    {
        const char *data_start = strstr(skip_buffer, "\r\n\r\n");

        if (!data_start)
        {
            ESP_LOGE(TAG_SERVER, "Multipart header start not found.");
            err = ESP_FAIL;
        }
        else
        {
            int data_offset = (data_start - skip_buffer) + 4;
            int first_chunk_data_size = skip_len - data_offset;
            size_t remaining = req->content_len - skip_len;

            if (first_chunk_data_size > 0)
            {
                if (fm->write_file_chunk(skip_buffer + data_offset, first_chunk_data_size) != ESP_OK)
                {
                    ESP_LOGE(TAG_SERVER, "Failed to write first chunk.");
                    err = ESP_FAIL;
                }
            }

            // --- 2. Loop for the rest of the file content ---
            char buf[DOWNLOAD_BUFFER_SIZE];

            while (err == ESP_OK && remaining > 0)
            {
                size_t chunk_size = std::min(remaining, sizeof(buf));

                if ((received_bytes = httpd_req_recv(req, buf, chunk_size)) <= 0)
                {
                    if (received_bytes == HTTPD_SOCK_ERR_TIMEOUT)
                        continue;
                    ESP_LOGE(TAG_SERVER, "File receive failed. Received bytes: %d", received_bytes);
                    err = ESP_FAIL;
                    break;
                }

                if (fm->write_file_chunk(buf, received_bytes) != ESP_OK)
                {
                    ESP_LOGE(TAG_SERVER, "Failed to write file chunk.");
                    err = ESP_FAIL;
                    break;
                }

                remaining -= received_bytes;
            }
        }
    }

    fm->close_file();

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG_SERVER, "File '%s' upload complete.", filename_start.c_str());
        httpd_resp_set_status(req, "303 See Other");
        httpd_resp_set_hdr(req, "Location", "/");
        httpd_resp_sendstr(req, "File uploaded successfully!");
    }
    else
    {
        fm->delete_file(filename_start.c_str());
        ESP_LOGE(TAG_SERVER, "File upload failed, file deleted.");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File transfer error.");
    }

    return err;
}

esp_err_t HttpFileServer::handle_delete_post(httpd_req_t *req)
{
    const char *uri_prefix = "/delete/";
    const char *filename_start_ptr = req->uri + strlen(uri_prefix);
    std::string filename(filename_start_ptr);

    if (filename.empty())
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing filename in URI.");
        return ESP_FAIL;
    }

    FileManager *fm = (FileManager *)req->user_ctx;

    if (fm->delete_file(filename.c_str()) == ESP_OK)
    {
        ESP_LOGI(TAG_SERVER, "File deleted: %s", filename.c_str());
        httpd_resp_set_status(req, "303 See Other");
        httpd_resp_set_hdr(req, "Location", "/");
        httpd_resp_sendstr(req, "File deleted successfully!");
    }
    else
    {
        ESP_LOGE(TAG_SERVER, "Failed to delete file: %s", filename.c_str());
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to delete file");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t HttpFileServer::handle_wifi_sta_config(httpd_req_t *req) {
    char ssid_buf[MAX_SSID_LEN + 1] = {0};
    char pass_buf[MAX_PASS_LEN + 1] = {0};
    std::string status_message;
    bool should_reboot = false;

    // Get WifiManager instance
    WifiManager& wifi_manager = WifiManager::getInstance();

    if (req->method == HTTP_POST) {
        char body_buf[256];
        int ret = httpd_req_recv(req, body_buf, sizeof(body_buf) - 1);
        if (ret > 0) {
            body_buf[ret] = '\0';
            char new_ssid[MAX_SSID_LEN + 1] = {0};
            char new_pass[MAX_PASS_LEN + 1] = {0};

            if (parse_post_param(body_buf, ret, "ssid", new_ssid, sizeof(new_ssid))) {
                // SSID is always present
                if (parse_post_param(body_buf, ret, "password", new_pass, sizeof(new_pass))) {
                    // Password is only present if the network is not open
                    if (nvs_manager.save_sta_credentials(new_ssid, new_pass) == ESP_OK) {
                        status_message = "<p class='success'>STA credentials saved! Device will attempt to connect after reboot.</p>";
                        should_reboot = true;
                    } else {
                        status_message = "<p class='error'>Failed to save STA credentials to NVS.</p>";
                    }
                } else {
                    // Open network, save empty password
                    if (nvs_manager.save_sta_credentials(new_ssid, "") == ESP_OK) {
                        status_message = "<p class='success'>STA credentials saved! Device will attempt to connect after reboot.</p>";
                        should_reboot = true;
                    } else {
                        status_message = "<p class='error'>Failed to save STA credentials to NVS.</p>";
                    }
                }
            } else {
                status_message = "<p class='error'>Missing SSID in form data.</p>";
            }
        } else {
            status_message = "<p class='error'>Failed to read request body.</p>";
        }
    }

    // Generate the config page with the list of networks
    std::string response_str = HtmlGenerator::generate_sta_config_page(status_message);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, response_str.c_str(), response_str.length());

    if (should_reboot) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart();
    }

    return ESP_OK;
}

esp_err_t HttpFileServer::handle_wifi_ap_config(httpd_req_t *req)
{
    char ssid_buf[MAX_SSID_LEN + 1] = {0};
    char pass_buf[MAX_PASS_LEN + 1] = {0};
    std::string status_message;
    bool should_reboot = false;

    nvs_manager.load_ap_credentials(ssid_buf, sizeof(ssid_buf), pass_buf, sizeof(pass_buf));

    if (req->method == HTTP_POST)
    {
        char body_buf[256];
        int ret = httpd_req_recv(req, body_buf, sizeof(body_buf));

        if (ret > 0)
        {
            char new_ssid[MAX_SSID_LEN + 1] = {0};
            char new_pass[MAX_PASS_LEN + 1] = {0};

            if (parse_post_param(body_buf, ret, "ssid", new_ssid, sizeof(new_ssid)) &&
                parse_post_param(body_buf, ret, "password", new_pass, sizeof(new_pass)))
            {

                if (strlen(new_pass) < 8)
                {
                    status_message = "<p class='error'>AP Password must be at least 8 characters long (WPA2 required).</p>";
                }
                else if (nvs_manager.save_ap_credentials(new_ssid, new_pass) == ESP_OK)
                {
                    status_message = "<p class='success'>AP credentials saved! Device will update AP settings after reboot.</p>";
                    should_reboot = true;
                }
                else
                {
                    status_message = "<p class='error'>Failed to save AP credentials to NVS.</p>";
                }
                strncpy(ssid_buf, new_ssid, sizeof(ssid_buf));
                strncpy(pass_buf, new_pass, sizeof(pass_buf));
            }
            else
            {
                status_message = "<p class='error'>Missing SSID or Password in form data.</p>";
            }
        }
        else
        {
            status_message = "<p class='error'>Failed to read request body.</p>";
        }
    }

    std::string response_str = HtmlGenerator::generate_ap_config_page(ssid_buf, pass_buf, status_message);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, response_str.c_str(), response_str.length());

    if (should_reboot)
    {
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart();
    }

    return ESP_OK;
}

// --- C-Style Handler Wrappers (External Interface) ---

// Static pointer to the singleton server instance, for C handlers to access
static HttpFileServer *global_server_instance = nullptr;

// Macro to cast the user_ctx back to the main server class instance.
#define DELEGATE_HANDLER(func_name)                                                              \
    do                                                                                           \
    {                                                                                            \
        if (!global_server_instance)                                                             \
        {                                                                                        \
            ESP_LOGE(TAG_SERVER, "Server not initialized.");                                     \
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Server not initialized"); \
            return ESP_FAIL;                                                                     \
        }                                                                                        \
        return global_server_instance->func_name(req);                                           \
    } while (0)

extern "C" esp_err_t root_handler(httpd_req_t *req)
{
    DELEGATE_HANDLER(handle_root_get);
}

extern "C" esp_err_t download_handler(httpd_req_t *req)
{
    DELEGATE_HANDLER(handle_download_get);
}

extern "C" esp_err_t upload_handler(httpd_req_t *req)
{
    DELEGATE_HANDLER(handle_upload_post);
}

extern "C" esp_err_t delete_handler(httpd_req_t *req)
{
    DELEGATE_HANDLER(handle_delete_post);
}

extern "C" esp_err_t wifi_sta_config_handler(httpd_req_t *req)
{
    DELEGATE_HANDLER(handle_wifi_sta_config);
}

extern "C" esp_err_t wifi_ap_config_handler(httpd_req_t *req)
{
    DELEGATE_HANDLER(handle_wifi_ap_config);
}

void HttpFileServer::run() {
    FileManager &file_manager = FileManager::getInstance();
    NvsConfigManager nvs_manager = NvsConfigManager::getInstance();
    WifiManager& wifi_manager = WifiManager::getInstance();

    // 1. Initialize SPIFFS
    if (file_manager.init_spiffs() != ESP_OK) {
        ESP_LOGE(TAG_SERVER, "Failed to initialize SPIFFS, cannot start server.");
        return;
    }

    // 2. Configure HTTP Server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = FILE_SERVER_TASK_STACK_SIZE;
    config.server_port = 80;
    config.uri_match_fn = httpd_uri_match_wildcard;

    // 3. Start the server and register handlers
    if (httpd_start(&server_handle, &config) == ESP_OK) {
        ESP_LOGI(TAG_SERVER, "HTTP File/Config Server started on port %d", config.server_port);

        esp_err_t err;

        // Note: The user_ctx for handlers needing file system access is set to FileManager::getInstance()
        // The handlers internally access global_server_instance which has a reference to the same manager.

        httpd_uri_t uris[] = {
            {.uri = "/upload/*", .method = HTTP_POST, .handler = upload_handler, .user_ctx = &file_manager},
            {.uri = "/delete/*", .method = HTTP_POST, .handler = delete_handler, .user_ctx = &file_manager},
            {.uri = "/", .method = HTTP_GET, .handler = root_handler, .user_ctx = &file_manager},
            {.uri = "/wifi/sta", .method = (httpd_method_t)HTTP_ANY, .handler = wifi_sta_config_handler, .user_ctx = &wifi_manager},
            {.uri = "/wifi/ap", .method = (httpd_method_t)HTTP_ANY, .handler = wifi_ap_config_handler, .user_ctx = NULL},
            {.uri = "/*", .method = HTTP_GET, .handler = download_handler, .user_ctx = &file_manager} // Catch-all for files
        };

        for (size_t i = 0; i < sizeof(uris) / sizeof(uris[0]); ++i) {
            if ((err = httpd_register_uri_handler(server_handle, &uris[i])) != ESP_OK) {
                ESP_LOGE(TAG_SERVER, "Failed to register handler %s: %s", uris[i].uri, esp_err_to_name(err));
            }
        }

        vTaskDelay(portMAX_DELAY);
    } else {
        ESP_LOGE(TAG_SERVER, "Error starting HTTP File Server!");
    }
}

void HttpFileServer::server_task(void *pvParameter)
{
    HttpFileServer *instance = (HttpFileServer *)pvParameter;
    instance->run();
    delete instance; // Cleanup instance when task exits (e.g., if initialization fails)
    global_server_instance = nullptr;
    vTaskDelete(NULL);
}

/**
 * @brief C entry point to start the HTTP server task.
 */
extern "C" void start_http_file_server()
{
    if (global_server_instance)
    {
        ESP_LOGW(TAG_SERVER, "HTTP Server already running.");
        return;
    }

    // Allocate the server instance
    global_server_instance = new HttpFileServer();

    if (!global_server_instance)
    {
        ESP_LOGE(TAG_SERVER, "Failed to allocate HttpFileServer instance.");
        return;
    }

    // Pass the instance to the task function
    if (xTaskCreate(
            HttpFileServer::server_task,
            "http_file_server",
            FILE_SERVER_TASK_STACK_SIZE,
            global_server_instance,
            5,
            NULL) != pdPASS)
    {
        ESP_LOGE(TAG_SERVER, "Failed to create server task.");
        delete global_server_instance;
        global_server_instance = nullptr;
    }
}
