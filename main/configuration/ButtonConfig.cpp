#include "ButtonConfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

static const char *TAG = "ButtonConfigReader";

// Structure to pass data to the task
typedef struct {
    std::string filePath;
    std::vector<ButtonConfig>* buttonConfigs;
} ParseTaskParams;

// Helper function to split a string by a delimiter
std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Task function for parsing CSV
void parseCsvTask(void *params) {
    ParseTaskParams* taskParams = (ParseTaskParams*)params;
    std::string filePath = taskParams->filePath;
    std::vector<ButtonConfig>* buttonConfigs = taskParams->buttonConfigs;

    ESP_LOGI(TAG, "Starting CSV parsing task.");

    std::ifstream file(filePath);
    if (!file.is_open()) {
        ESP_LOGE(TAG, "Failed to open file: %s", filePath.c_str());
        vTaskDelete(NULL); // Delete the task
        return;
    }

    ESP_LOGI(TAG, "Successfully opened file: %s", filePath.c_str());

    std::string line;
    std::getline(file, line); // Skip the header line

    try {
        buttonConfigs->clear(); // Clear existing configs before parsing

        while (std::getline(file, line)) {
            std::vector<std::string> tokens = splitString(line, ',');
            if (tokens.size() != 5) {
                ESP_LOGE(TAG, "Invalid CSV line: %s", line.c_str());
                continue; // Skip invalid lines
            }

            ButtonConfig config;
            config.id = std::stoi(tokens[0]);
            config.label = tokens[1];
            config.iconPath = tokens[2];
            config.modifier = static_cast<uint8_t>(std::stoi(tokens[3], nullptr, 16)); // Parse as hexadecimal
            config.keycode = static_cast<uint8_t>(std::stoi(tokens[4], nullptr, 16));  // Parse as hexadecimal

            buttonConfigs->push_back(config);
        }

        ESP_LOGI(TAG, "Successfully parsed button configuration CSV file.");
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Exception: %s", e.what());
    }

    vTaskDelete(NULL); // Delete the task when done
}

ButtonConfigReader::ButtonConfigReader(const std::string& filePath) : filePath(filePath) {
    buttonConfigs = new std::vector<ButtonConfig>(); // Allocate buttonConfigs

    ParseTaskParams* taskParams = new ParseTaskParams();
    taskParams->filePath = filePath;
    taskParams->buttonConfigs = buttonConfigs;

    BaseType_t result = xTaskCreate(
        parseCsvTask,           // Task function
        "CsvParseTask",         // Task name
        4096,                   // Stack size (adjust as needed)
        taskParams,             // Task parameters
        5,                      // Task priority
        NULL                    // Task handle
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create CSV parsing task.");
        // Handle error appropriately (e.g., set a flag, use default configs)
        delete taskParams; // Free memory
        delete buttonConfigs;
        buttonConfigs =  nullptr; // Ensure it's a nullptr
    }
}

ButtonConfigReader::~ButtonConfigReader() {
    // Cleanup allocated memory
    delete buttonConfigs;
}

std::vector<ButtonConfig> ButtonConfigReader::getButtonConfigs() {
    // Return a copy to avoid external modification
    return *buttonConfigs;
}