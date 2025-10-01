#ifndef BUTTONCONFIG_H
#define BUTTONCONFIG_H

#include <string>
#include <vector>

class ButtonConfig { // Changed to class
public: // Added public access specifier
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

private:
    std::string filePath;
    std::vector<ButtonConfig>* buttonConfigs; // Changed to pointer
};

#endif