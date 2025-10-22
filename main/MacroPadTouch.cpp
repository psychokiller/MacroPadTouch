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
#include "fonts/fonts.h"
#include "graphics/Button.h"
#include "ui/UiManager.h"
#include <string>
#include <vector>
#include "bluetooth/BleKeyboard.h"
#include "configuration/ButtonConfig.h"
#include "graphics/IconLoader.h"
#include "esp_spiffs.h"
#include "services/HttpFileServer.hpp"
#include "WifiManager.hpp"
#include "ui/OnScreenKeyboard.hpp"

static const char *TAG_APP = "Main";

i2c_master_dev_handle_t *i2c_dev_handle, i2c_dev;

void setup_i2c_configuration(TouchDriver *);
void clear_screen(uint8_t *BlankDisplayImage, Display &display, display_color color);

// Global pointer to the server instance to manage its lifetime/state
static HttpFileServer *s_http_server = nullptr;

// Custom Event Handler for the Application
static void app_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
    if (event_base == APP_EVENTS) {
        // Condition 1: Start the server when ANY networking mode is ready.
        if (event_id == APP_EVENT_WIFI_STA_CONNECTED || event_id == APP_EVENT_WIFI_AP_STARTED) {
            if (s_http_server == nullptr) {
                ESP_LOGI(TAG_APP, "Network ready. Starting HTTP Server...");
                WifiManager& wifi_manager = WifiManager::getInstance();
                start_http_file_server();
            }
        }
        
        // Condition 2: Handle server shutdown if STA connection is lost (optional)
        if (event_id == APP_EVENT_WIFI_DISCONNECTED) {
             // You could implement logic here to temporarily stop the server 
             // or switch its behavior, but generally, the AP keeps it running.
        }
    }
}


extern "C" void app_main(void)
{
    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    NvsConfigManager nvs_manager_instance = NvsConfigManager::getInstance();

    ESP_ERROR_CHECK(esp_event_handler_register(APP_EVENTS, ESP_EVENT_ANY_ID, &app_event_handler, NULL));

    WifiManager& wifi_manager = WifiManager::getInstance();
    ESP_ERROR_CHECK(wifi_manager.start_apsta());


    BleKeyboard bleKeyboard("MacroPadTouch");

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

    UiManager *ui = new UiManager(display, 2, 3);
    
    std::vector<Button *> buttons;

    IconLoader iconLoader;

    ESP_LOGI("Main", "Loading button configurations from CSV file.");
     // Load button configurations from JSON file
    ButtonConfigReader configReader("/spiffs/btn_config.csv");
    ESP_LOGI("Main", "Loaded button configurations from CSV file.");
    std::vector<ButtonConfig> buttonConfigs = configReader.getButtonConfigs();

    // Create buttons based on configurations
    for (const auto& config : buttonConfigs) {
        // Load icon (replace with your actual icon loading logic)
        const unsigned char* icon = iconLoader.loadIcon(config.iconPath);

        buttons.push_back(new Button(config.label, &Font20, BLACK, WHITE, icon, config.modifier, config.keycode));
    }

    ui->draw(buttons);

    display.display(BlankDisplayImage);

    TouchPoint last_tp;
    uint32_t last_touch_time = 0;
    const uint32_t debounce_ms = 200; // 200ms debounce

    while (true)
    {
        TouchPoint tp = touchDriver->scan(i2c_dev_handle);
        TouchPoint aTp = ui->getPressedCoordinates(&tp);
        // Keyboard_HandleTouch(aTp.x, aTp.y);

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
                Button* pressedButton = ui->getPressedButton(&tp, buttons);
                if (pressedButton != nullptr)
                {
                    last_touch_time = current_time;
                    uint8_t btnIndex = pressedButton->get_index();
                    std::string btnText = pressedButton->get_label();
                    ESP_LOGI("Main", "Button %s Idx: %d pressed!", btnText.c_str(), btnIndex);

                    bleKeyboard.sendKey(pressedButton->get_modifier(), pressedButton->get_keycode());
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
