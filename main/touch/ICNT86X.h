#ifndef _ICNT86X_H_
#define _ICNT86XH_

#include "TouchDriver.h"

class Icnt86x : public TouchDriver
{
private:
    const char *TAG = "Touch_Driver_ICNT86X";
    const int maxTouchPoints = 10;
    const uint16_t chip_address = 0x48;
    const uint16_t productId_reg = 0x000A;
    const uint16_t baseCoordinates_address = 0x1001;
    const uint16_t firstCoordinates_address = 0x1002;
    
public:
    TouchPoint touch;

    Icnt86x(/* args */);
    ~Icnt86x();

    void reset() override;
    esp_err_t read_product_id(i2c_master_dev_handle_t *) override;
    void init(i2c_master_dev_handle_t *) override;
    TouchPoint scan(i2c_master_dev_handle_t *) override;

    i2c_device_config_t get_device_config() override;
};

#endif