#include "BleKeyboard.h"
#include "esp_log.h"
#include "nvs_flash.h" // Include for NVS functions
#include "NimBLEDevice.h"


static const char *B_KB_TAG = "BLE_KEYBOARD";

class ServerCallbacks : public NimBLEServerCallbacks {

    public:
        ServerCallbacks() {}

        void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
            ESP_LOGI(B_KB_TAG, "Client connected");
        };

        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
            ESP_LOGI(B_KB_TAG, "Client disconnected - start advertising");
            pServer->startAdvertising();
        };
};

const uint8_t BleKeyboard::reportMap[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report ID (1)
    0x05, 0x07,       // Usage Page (Key Codes)
    0x19, 0xE0,       // Usage Minimum (224)
    0x29, 0xE7,       // Usage Maximum (231)
    0x15, 0x00,       // Logical Minimum (0)
    0x25, 0x01,       // Logical Maximum (1)
    0x75, 0x01,       // Report Size (1)
    0x95, 0x08,       // Report Count (8)
    0x81, 0x02,       // Input (Data, Variable, Absolute) ; Modifier byte
    0x95, 0x01,       // Report Count (1)
    0x75, 0x08,       // Report Size (8)
    0x81, 0x01,       // Input (Constant) ; Reserved byte
    0x95, 0x05,       // Report Count (6)
    0x75, 0x01,       // Report Size (8)
    0x05, 0x08,       // Usage Page (LEDs)
    0x19, 0x01,       // Usage Minimum (1)
    0x29, 0x05,       // Usage Maximum (5)
    0x91, 0x02,       // Output (Data, Variable, Absolute) ; LED report
    0x95, 0x01,       // Report Count (1)
    0x75, 0x03,       // Report Size (3)
    0x91, 0x01,       // Output (Constant) ; LED report padding
    0x95, 0x06,       // Report Count (6)
    0x75, 0x08,       // Report Size (8)
    0x15, 0x00,       // Logical Minimum (0)
    0x25, 0x65,       // Logical Maximum (101)
    0x05, 0x07,       // Usage Page (Key Codes)
    0x19, 0x00,       // Usage Minimum (0)
    0x29, 0x65,       // Usage Maximum (101)
    0x81, 0x00,       // Input (Data, Array)
    0xC0              // End Collection
};

BleKeyboard::BleKeyboard(const std::string& devName) {
    deviceName = devName;

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    NimBLEDevice::init(deviceName);
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    pServer = NimBLEDevice::createServer();
    pServer->advertiseOnDisconnect(true);
    pServer->setCallbacks(new ServerCallbacks());
    hid = new NimBLEHIDDevice(pServer);
    input = hid->getInputReport(1);   // Report ID = 1
    pAdvertising = NimBLEDevice::getAdvertising();

    NimBLEDevice::init(deviceName);

    hid->setManufacturer("Smile's Co.");

    hid->setPnp(0x02, 0xe502, 0xa111, 0x0210);
    hid->setHidInfo(0x0111, 0x01);

    hid->setReportMap((uint8_t*)reportMap, sizeof(reportMap));
    hid->setBatteryLevel(88);
    hid->startServices();

    pAdvertising->addServiceUUID(NimBLEUUID("180F")); // Battery Service UUID
    pAdvertising->addServiceUUID(hid->getHidService()->getUUID());

    pAdvertising->setAppearance(HID_KEYBOARD);
    pAdvertising->setName(deviceName);
    pAdvertising->setMinInterval(32); // Reduced advertising interval (20ms)
    pAdvertising->setMaxInterval(64); // Reduced advertising interval (40ms)
    pAdvertising->start();
    sendKey(0x00, 0x00); // Send empty report to avoid stuck keys
}

bool BleKeyboard::isConnected() {
    return pServer->getConnectedCount() > 0;
}

void BleKeyboard::sendKey(uint8_t modifiers, uint8_t keycode) {
    uint8_t report[8] = {0};
    
    // Release key
    memset(report, 0, sizeof(report));
    input->setValue(report, sizeof(report));

    input->notify();

    vTaskDelay(100 / portTICK_PERIOD_MS); // 10 ms

    
    report[0] = modifiers; // Modifier keys
    report[2] = keycode;   // Put key in first slot
    input->setValue(report, sizeof(report));

    input->notify();

    vTaskDelay(10 / portTICK_PERIOD_MS); // 10 ms

    releaseAllKeys();
}

void BleKeyboard::releaseAllKeys() {
    uint8_t report[8] = {0};

    // Release key
    memset(report, 0, sizeof(report));
    input->setValue(report, sizeof(report));

    input->notify();
}


