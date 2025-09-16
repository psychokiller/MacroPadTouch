#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "touch/GT1151.h"
#include "touch/ICNT86X.h"
#include "touch/TouchPoint.h"
#include "Config.h"
#include "Utils.h"
#include "display/ePaper/WaveShare213.h"
#include "display/ePaper/WaveShare29.h"
#include "driver/spi_master.h"
#include "paint/GUI_Paint.h"
#include "graphics/ImageData.h"
#include "fonts/fonts.h"
#include "graphics/Button.h"
#include "ui/UiManager.h"
#include <string>
#include <vector>
#include "NimBLEDevice.h"
#include "NimBLEHIDDevice.h"
#include "NimBLEServer.h"

i2c_master_dev_handle_t *i2c_dev_handle, i2c_dev;



void setup_i2c_configuration(TouchDriver *);
void clear_screen(uint8_t *BlankDisplayImage, Display &display, display_color color);

NimBLEServer* pServer;
NimBLEHIDDevice* hid;
NimBLECharacteristic* input;
NimBLEAdvertising* pAdvertising;

void sendKey(uint8_t keycode) {
    uint8_t report[8] = {0};
    report[2] = keycode;   // Put key in first slot
    input->setValue(report, sizeof(report));
    input->notify();

    // Release key
    memset(report, 0, sizeof(report));
    input->setValue(report, sizeof(report));
    input->notify();
}

void setupBLE() {
    NimBLEDevice::init("MacroPadTouch");

    pServer = NimBLEDevice::createServer();
    hid = new NimBLEHIDDevice(pServer);

    input = hid->getInputReport(1);   // Report ID = 1
    hid->setManufacturer("ESP32");

    hid->setPnp(0x02, 0xe502, 0xa111, 0x0210);
    hid->setHidInfo(0x00,0x01);

    const uint8_t reportMap[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x05, 0x07,       // Usage Page (Keyboard/Keypad)
    0x19, 0xE0,       // Usage Minimum (224)
    0x29, 0xE7,       // Usage Maximum (231)
    0x15, 0x00,       // Logical Minimum (0)
    0x25, 0x01,       // Logical Maximum (1)
    0x75, 0x01,       // Report Size (1)
    0x95, 0x08,       // Report Count (8)
    0x81, 0x02,       // Input (Modifier)
    0x95, 0x01,       // Report Count (1)
    0x75, 0x08,       // Report Size (8)
    0x81, 0x01,       // Input (Reserved)
    0x95, 0x06,       // Report Count (6)
    0x75, 0x08,       // Report Size (8)
    0x15, 0x00,       // Logical Minimum (0)
    0x25, 0x65,       // Logical Maximum (101)
    0x05, 0x07,       // Usage Page (Keyboard/Keypad)
    0x19, 0x00,       // Usage Minimum (0)
    0x29, 0x65,       // Usage Maximum (101)
    0x81, 0x00,       // Input (Keys)
    0xC0              // End Collection
};


    hid->setReportMap((uint8_t*)reportMap, sizeof(reportMap));
    hid->startServices();

    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setAppearance(HID_KEYBOARD);
    pAdvertising->addServiceUUID(hid->getHidService()->getUUID());
    pAdvertising->start();
}

extern "C" void app_main(void)
{
    setupBLE();
    TouchDriver *touchDriver = new Gt1151();
    // TouchDriver* touchDriver = new Icnt86x();
    setup_i2c_configuration(touchDriver);
    touchDriver->init(i2c_dev_handle);
    vTaskDelay(pdMS_TO_TICKS(500));

    WaveShare213 display = WaveShare213();
    // WaveShare29 display = WaveShare29();

    display.init();
    display.clear(BLACK);

    // Create a new image cache
    uint8_t *BlankDisplayImage;
    uint16_t Imagesize = display.get_screen_size_bytes(); // should be Display Width * Height (in bytes not pixels)
    if ((BlankDisplayImage = (uint8_t *)malloc(Imagesize)) == NULL)
    {
        printf("Failed to apply for black memory...\r\n");
        while (1)
            ;
    }

    clear_screen(BlankDisplayImage, display, WHITE);
    Paint_SetMirroring(MIRROR_ORIGIN);

    const uint16_t number_of_buttons = 6;

    UiManager *ui = new UiManager(display, 2, 3);
    std::vector<Button *> buttons;

    // Button** buttons = new Button*[number_of_buttons];

    for (int i = 0; i < number_of_buttons; i++)
    {
        const char *btn_header = ("KOKO" + std::to_string(i)).c_str();
        ESP_LOGI("Main", "Btn Header: %s", btn_header);
        buttons.push_back(new Button(const_cast<char *>(btn_header), &Font20, BLACK, WHITE));
    }

    ui->draw(buttons);

    display.display(BlankDisplayImage);

    TouchPoint last_tp;
    uint32_t last_touch_time = 0;
    const uint32_t debounce_ms = 200; // 200ms debounce

    while (true)
    {
        TouchPoint tp = touchDriver->scan(i2c_dev_handle);

        if (tp.size > 0)
        {
            uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

            // Simple debouncing - ignore touches too close in time and position
            if (current_time - last_touch_time > debounce_ms ||
                abs((int)tp.x - (int)last_tp.x) > 10 ||
                abs((int)tp.y - (int)last_tp.y) > 10)
            {

                last_tp = tp;
                // use tp
                if (ui->isButtonPressed(&tp, buttons))
                {
                }
            }
        }
    }
}

void clear_screen(uint8_t *BlankDisplayImage, Display &display, display_color color)
{
    Paint_NewImage(BlankDisplayImage, display.get_width(), display.get_height(), ROTATE_90, color);
    Paint_Clear(color);
}

void setup_i2c_configuration(TouchDriver *td)
{
    // define I2C bus configuration
    i2c_master_bus_config_t master_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags = {
            .enable_internal_pullup = true}};

    i2c_master_bus_handle_t bus_handle;
    i2c_new_master_bus(&master_config, &bus_handle);

    i2c_device_config_t slave_dev_cnfg = td->get_device_config();

    i2c_dev_handle = &i2c_dev;

    i2c_master_bus_add_device(bus_handle, &slave_dev_cnfg, i2c_dev_handle);

    // configure the TOUCH_INT pin
    gpio_config_t touch_interrupt_pin_config = {
        .pin_bit_mask = TOUCH_INT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config(&touch_interrupt_pin_config);

    // configure the TOUCH_RST pin
    gpio_config_t reset_pin_config = {
        .pin_bit_mask = TOUCH_RST,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&reset_pin_config);
}
