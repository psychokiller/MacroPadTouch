#include "IconLoader.h"
#include "esp_log.h"
#include <fstream>
#include <iostream>
#include <vector>

static const char *TAG = "IconLoader";

IconLoader::IconLoader() {}

IconLoader::~IconLoader() {
    // Release memory for loaded icons
    for (const unsigned char* icon : loadedIcons) {
        delete[] icon;
    }
    loadedIcons.clear();
}

const unsigned char* IconLoader::loadIcon(const std::string& iconPath) {
    std::ifstream file(iconPath, std::ios::binary);
    if (!file.is_open()) {
        ESP_LOGE(TAG, "Failed to open icon file: %s", iconPath.c_str());
        return nullptr;
    }

    // Read BMP header (54 bytes)
    unsigned char header[54];
    file.read((char*)header, 54);

    // Check BMP signature
    if (header[0] != 'B' || header[1] != 'M') {
        ESP_LOGE(TAG, "Invalid BMP file: %s", iconPath.c_str());
        return nullptr;
    }

    // Extract image information
    int width = *(int*)&header[18];
    int height = *(int*)&header[22];
    int dataOffset = *(int*)&header[10];
    int bitsPerPixel = *(int*)&header[28];

    // Validate image dimensions (max 32x32)
    if (width > 32 || height > 32) {
        ESP_LOGE(TAG, "Icon dimensions exceed 32x32: %s (%dx%d)", iconPath.c_str(), width, height);
        return nullptr;
    }

    // Check if the BMP is 1-bit (uncompressed and monochrome)
    if (bitsPerPixel != 1) {
        ESP_LOGE(TAG, "Only 1-bit BMPs are supported: %s", iconPath.c_str());
        return nullptr;
    }

    // Calculate image size
    int imageSize = width * height * 3; // 3 bytes per pixel (RGB)

    // Allocate memory for pixel data
    unsigned char* iconData = new unsigned char[imageSize];

    // Read pixel data
    file.seekg(dataOffset, std::ios::beg); // Set position to start of pixel data
    file.read((char*)iconData, imageSize);

    // BMPs are stored upside down, so you might need to flip the data
    // (Implementation of flipping is left as an exercise)

    ESP_LOGI(TAG, "Successfully loaded icon: %s (%dx%d)", iconPath.c_str(), width, height);
    return iconData;
}