#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <string>
#include <vector>       // Added for file listing
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <cerrno>       // Added for better error logging in delete_file
#include <dirent.h>     // Added for opendir/readdir

/**
 * @brief Manages all file system (SPIFFS) operations as a Singleton, 
 * ensuring only one instance controls the file system state.
 */
class FileManager {
public:
    /**
     * @brief Structure to hold file metadata.
     */
    struct FileInfo {
        bool exists = false;
        size_t size = 0;
    };

private:
    std::string partition_label_ = "spiffs";
    std::string base_path_;
    // File pointer used during active upload/download stream
    FILE* current_file_handle_; 
    
    const char* TAG = "FILE_MANAGER";

    FileManager() 
        : current_file_handle_(nullptr) {
        base_path_ = "/" + partition_label_;
    }

    // Prevent copy and assignment
    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;

    /**
     * @brief Helper to convert a user-provided filename (e.g., "config.txt")
     * into the full VFS path (e.g., "/spiffs/config.txt").
     */
    std::string get_full_path(const char* filename) const {
        return base_path_ + "/" + filename;
    }

public:
    static FileManager& getInstance() {
        // Static local variable guarantees single, thread-safe initialization (C++11+)
        static FileManager instance;
        return instance;
    }

    ~FileManager() {
        close_file();
    }

    // --- FS Initialization ---

    esp_err_t init_spiffs() {
        ESP_LOGI(TAG, "Initializing SPIFFS...");

        esp_vfs_spiffs_conf_t conf = {
            .base_path = base_path_.c_str(),
            .partition_label = partition_label_.c_str(),
            .max_files = 5,
            .format_if_mount_failed = true
        };

        esp_err_t ret = esp_vfs_spiffs_register(&conf);

        if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "SPIFFS already mounted.");
            return ESP_OK;
        }
        
        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                ESP_LOGE(TAG, "Failed to mount or format SPIFFS");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                ESP_LOGE(TAG, "Failed to find SPIFFS partition");
            } else {
                ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
            }
            return ret;
        }

        size_t total = 0, used = 0;
        ret = esp_spiffs_info(partition_label_.c_str(), &total, &used); 
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "SPIFFS mounted. Total: %zu bytes, Used: %zu bytes", total, used);
        }
        
        return ESP_OK;
    }

    // --- File Metadata ---

    /**
     * @brief Checks if a file exists and returns its metadata.
     */
    FileInfo get_file_info(const char* filename) {
        std::string full_path = get_full_path(filename);
        FileInfo info;
        struct stat st;
        // Use stat to check existence and size
        if (stat(full_path.c_str(), &st) == 0) {
            info.exists = true;
            info.size = st.st_size;
        }
        return info;
    }

    // --- File Listing (NEW) ---

    /**
     * @brief Lists all files present in the SPIFFS partition base path.
     * @return A vector of strings, where each string is a filename (relative to mount point).
     */
    std::vector<std::string> list_files() {
        std::vector<std::string> file_list;
        // Use the base_path_ (e.g., "/spiffs") for opendir
        DIR* dir = opendir(base_path_.c_str()); 
        
        if (!dir) {
            ESP_LOGE(TAG, "Failed to open directory: %s", base_path_.c_str());
            return file_list;
        }

        struct dirent* entry;
        // Read directory entries one by one
        while ((entry = readdir(dir)) != NULL) {
            // Ignore special entries
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            // For flat filesystems like SPIFFS, all entries here should be files.
            file_list.push_back(entry->d_name);
        }

        closedir(dir);
        ESP_LOGI(TAG, "File list retrieved. Found %zu files.", file_list.size());
        return file_list;
    }

    // --- Write/Upload Methods ---

    esp_err_t open_file_for_write(const char* filename, size_t expected_size) {
        if (current_file_handle_) {
            ESP_LOGE(TAG, "File handle already open. Concurrency error.");
            return ESP_FAIL; 
        }

        std::string full_path = get_full_path(filename);
        current_file_handle_ = fopen(full_path.c_str(), "wb");

        if (!current_file_handle_) {
            ESP_LOGE(TAG, "Failed to open file for writing: %s", full_path.c_str());
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Opened file: %s for writing (%zu bytes expected)", full_path.c_str(), expected_size);
        return ESP_OK;
    }

    esp_err_t write_file_chunk(const char* data, size_t len) {
        if (!current_file_handle_) {
            ESP_LOGE(TAG, "No file open to write chunk to!");
            return ESP_FAIL;
        }

        size_t written = fwrite(data, 1, len, current_file_handle_);
        if (written != len) {
            ESP_LOGE(TAG, "File write failed (Wrote %zu/%zu bytes)", written, len);
            return ESP_FAIL;
        }
        return ESP_OK;
    }

    // --- Read/Download Methods ---

    /**
     * @brief Opens a file for reading.
     */
    esp_err_t open_file_for_read(const char* filename) {
        if (current_file_handle_) {
            ESP_LOGE(TAG, "File handle already open. Concurrency error.");
            return ESP_FAIL; 
        }

        std::string full_path = get_full_path(filename);
        current_file_handle_ = fopen(full_path.c_str(), "rb");

        if (!current_file_handle_) {
            ESP_LOGE(TAG, "Failed to open file for reading: %s", full_path.c_str());
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Opened file: %s for reading", full_path.c_str());
        return ESP_OK;
    }

    /**
     * @brief Reads a chunk of data from the currently open file handle.
     */
    int read_file_chunk(char* buf, size_t len) {
        if (!current_file_handle_) {
            ESP_LOGE(TAG, "No file open to read chunk from!");
            return -1;
        }
        size_t read_bytes = fread(buf, 1, len, current_file_handle_);

        if (ferror(current_file_handle_)) {
            ESP_LOGE(TAG, "File read error occurred (errno: %d).", errno);
            return -1;
        }
        return (int)read_bytes;
    }

    // --- Close and Delete ---

    void close_file() {
        if (current_file_handle_) {
            fclose(current_file_handle_);
            current_file_handle_ = nullptr;
            ESP_LOGI(TAG, "File handle closed.");
        }
    }

    esp_err_t delete_file(const char* filename) {
        std::string full_path = get_full_path(filename);
        if (unlink(full_path.c_str()) == 0) {
            ESP_LOGW(TAG, "Deleted file: %s", full_path.c_str());
            return ESP_OK;
        } else {
            ESP_LOGE(TAG, "Failed to delete file: %s (errno: %d)", full_path.c_str(), errno); 
            return ESP_FAIL;
        }
    }
};
