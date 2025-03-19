#ifndef _GT1151_H_
#define _GT1151_H_

#include "Config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include <string.h>
#include "Utils.h"
#include "TouchPoint.h"
#include "TouchDriver.h"

class Gt1151 : public TouchDriver
{
private:
    const char *TAG = "Touch_Driver_GT1151";
    const int maxTouchPoints = 10;
    const uint16_t chip_address = 0x14;
    const uint16_t productId_reg = 0x8140;
    const uint16_t baseCoordinates_address = 0x814E;
    const uint16_t firstCoordinates_address = 0x814F;
    
public:
    TouchPoint touch;

    Gt1151(/* args */);
    ~Gt1151();

    void reset() override;
    esp_err_t read_product_id(i2c_master_dev_handle_t *) override;
    void init(i2c_master_dev_handle_t *) override;
    TouchPoint scan(i2c_master_dev_handle_t *) override;

    i2c_device_config_t get_device_config() override;
};


#endif