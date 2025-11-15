#ifndef BUTTONCONFIG_H
#define BUTTONCONFIG_H

#include <string>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class ButtonConfig {
public:
    int id;
    std::string label;
    std::string iconPath;
    uint8_t modifier;
    uint8_t keycode;
};

class ButtonConfigReader {
public:
    ButtonConfigReader(const std::string& filePath);
    ~ButtonConfigReader();
    std::vector<ButtonConfig> getButtonConfigs();
    bool loadAndRetryUntilComplete();

private:
    std::string filePath;
    std::vector<ButtonConfig>* buttonConfigs;
    SemaphoreHandle_t parsingSemaphore;
    static const int EXPECTED_BUTTON_COUNT = 6;

    void startReading();
    bool waitForCompletion(TickType_t xTicksToWait);
    int getExpectedButtonCount();
    static void parseCsvTask(void *params);
};

#endif
