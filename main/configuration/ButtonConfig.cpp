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

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void ButtonConfigReader::parseCsvTask(void *params) {
    ButtonConfigReader* reader = (ButtonConfigReader*)params;
    
    ESP_LOGI(TAG, "Starting CSV parsing task.");

    std::ifstream file(reader->filePath);
    if (!file.is_open()) {
        ESP_LOGE(TAG, "Failed to open file: %s", reader->filePath.c_str());
        xSemaphoreGive(reader->parsingSemaphore);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Successfully opened file: %s", reader->filePath.c_str());

    std::string line;
    std::getline(file, line);

    try {
        reader->buttonConfigs->clear();

        while (std::getline(file, line)) {
            std::vector<std::string> tokens = splitString(line, ',');
            if (tokens.size() != 5) {
                ESP_LOGE(TAG, "Invalid CSV line: %s", line.c_str());
                continue;
            }

            ButtonConfig config;
            config.id = std::stoi(tokens[0]);
            config.label = tokens[1];
            config.iconPath = tokens[2];
            config.modifier = static_cast<uint8_t>(std::stoi(tokens[3], nullptr, 16));
            config.keycode = static_cast<uint8_t>(std::stoi(tokens[4], nullptr, 16));

            reader->buttonConfigs->push_back(config);
        }

        ESP_LOGI(TAG, "Successfully parsed button configuration CSV file.");
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Exception: %s", e.what());
    }

    xSemaphoreGive(reader->parsingSemaphore);
    vTaskDelete(NULL);
}

ButtonConfigReader::ButtonConfigReader(const std::string& filePath) : filePath(filePath) {
    buttonConfigs = new std::vector<ButtonConfig>();
    parsingSemaphore = xSemaphoreCreateBinary();
}

ButtonConfigReader::~ButtonConfigReader() {
    delete buttonConfigs;
    vSemaphoreDelete(parsingSemaphore);
}

void ButtonConfigReader::startReading() {
    BaseType_t result = xTaskCreate(
        parseCsvTask,
        "CsvParseTask",
        4096,
        this,
        5,
        NULL
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create CSV parsing task.");
    }
}

bool ButtonConfigReader::waitForCompletion(TickType_t xTicksToWait) {
    return xSemaphoreTake(parsingSemaphore, xTicksToWait) == pdTRUE;
}

int ButtonConfigReader::getExpectedButtonCount() {
    return EXPECTED_BUTTON_COUNT;
}

std::vector<ButtonConfig> ButtonConfigReader::getButtonConfigs() {
    return *buttonConfigs;
}

bool ButtonConfigReader::loadAndRetryUntilComplete() {
    do {
        ESP_LOGI(TAG, "Starting to read button configurations...");
        startReading();

        if (waitForCompletion(portMAX_DELAY)) {
            ESP_LOGI(TAG, "Configuration reading process finished.");
            if (getButtonConfigs().size() < getExpectedButtonCount()) {
                 ESP_LOGW(TAG, "Incomplete button configurations. Expected %d, but got %d. Retrying...",
                         getExpectedButtonCount(), getButtonConfigs().size());
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        } else {
            ESP_LOGE(TAG, "Failed to read button configurations. Retrying...");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    } while (getButtonConfigs().size() < getExpectedButtonCount());
    
    ESP_LOGI(TAG, "Successfully loaded all button configurations.");
    return true;
}
