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

#define GT1151N_ADDR 0x14
#define PRODUCT_ID_REG 0x8140
#define BASE_COORD_ADDR 0x814E
#define FIRST_COORD_ADDR 0x814F
#define MAX_TOUCH_POINTS 10

void reset();
esp_err_t read_product_id(i2c_master_dev_handle_t*);
void init(i2c_master_dev_handle_t*);
TouchPoint scan(i2c_master_dev_handle_t*);

i2c_device_config_t get_device_config();

#endif