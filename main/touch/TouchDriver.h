#ifndef _TouchDriver_H_
#define _TouchDriver_H_

#include "Config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include <string.h>
#include "Utils.h"
#include "TouchPoint.h"

class TouchDriver
{
public:
    TouchDriver();
    TouchPoint touch;
    virtual void reset() = 0;
    virtual esp_err_t read_product_id(i2c_master_dev_handle_t *) = 0;
    virtual void init(i2c_master_dev_handle_t *) = 0;
    virtual TouchPoint scan(i2c_master_dev_handle_t *) = 0;

    virtual i2c_device_config_t get_device_config() = 0;
};

#endif