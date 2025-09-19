#pragma once

#include <string>
#include <cstdint>

#include "NimBLEDevice.h"
#include "NimBLEHIDDevice.h"
#include "NimBLEServer.h"

class BleKeyboard{
public:
    BleKeyboard(const std::string& deviceName = "MacroPadTouch");

    bool isConnected();
    void sendKey(uint8_t modifiers, uint8_t keycode);
    void releaseAllKeys();
private:
    NimBLEServer* pServer;
    NimBLEHIDDevice* hid;
    NimBLECharacteristic* input;
    NimBLEAdvertising* pAdvertising;
    std::string deviceName;
    
    void setupBLE();
    
    static const uint8_t reportMap[];
};
