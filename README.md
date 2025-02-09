# MacroPadTouch
An E-Paper based MacroPad that is based on ESP32 or RPI Pico, with multiple features

## Main Features
This is a comprehensive wish list, doesn't mean they will all come to life :D

1. Touch based macropad that is reliable as much as any of the physical mechanical keebs
   1. Which layer of keys are active
   2. Icons for the keys
3. Can be used for Pomodoro
   1. Enable Focus mode on iPhone/Mac
   2. Timer with settings
4. Stat Feeds
   1. CPU Utilization
   2. Memory Usage
   3. Disk IO
   4. LAN IO
   5. Wifi name
   6. Wifi IO
5. Customizable Feeds
   1. Adding hooks to other sources (this needs a bit of design to define a constant way of doing it)

## Hardware
1. ESP32-S2/3
2. RPI Pico W or RPI Pico 2
3. Waveshare ePaper Touch 2.13" and 2.9" (GT1151N and ICNT86X touch sensors)

### References
1. Waveshare 2.13" schematic https://files.waveshare.com/upload/1/10/2.13inch_Touch_e-Paper_HAT.pdf
2. Waveshare ePaper driver ESP32 board https://www.waveshare.com/wiki/E-Paper_ESP32_Driver_Board
3. Touch sensor datasheets are in the repo

## Pin Mappings
### ESP32 -> RPI 40Pin Connector
This maps esp32 pins to the 40pin connector pin number, and the GPIO Pin Number


| ESP32 Pin | 40 Pin Connector Pin | RPI GPIO Pin | Pin Function |
|:--:|:--:|:--:|:--:|
| 26 | 11 | 17 | DISPLAY_RESET |
| 27 | 22 | 25 | DISPLAY_SPI_DC|
| 15 | 24 | 8 | DISPLAY_SPI_CS|
| 25 | 18 | 24 | DISPLAY_BUSY|
| 21 | 3 | 2 | TOUCH_I2C_SDA |
| 22 | 5 | 3 | TOUCH_I2C_SCL |
| 16 | 15 | 22 | TOUCH_RESET|
| 17 | 13 | 27 | TOUCH_INTERRUPT|
| 14 | 19 | 10 |  DISPLAY_SPI_MOSI|
| 13 | 23 | 11 | DISPLAY_SPI_CLK|

### PICO -> RPI 40Pin Connector

| PICO 2 GPIO Pin | 40 Pin Connector Pin | RPI GPIO Pin | Pin Function |
|:--:|:--:|:--:|:--:|
| 12 | 11 | 17 | DISPLAY_RESET |
| 8 | 22 | 25 | DISPLAY_SPI_DC|
| 9 | 24 | 8 | DISPLAY_SPI_CS|
| 13 | 18 | 24 | DISPLAY_BUSY|
| 6 | 3 | 2 | TOUCH_I2C_SDA |
| 7 | 5 | 3 | TOUCH_I2C_SCL |
| 16 | 15 | 22 | TOUCH_RESET|
| 17 | 13 | 27 | TOUCH_INTERRUPT|
| 11 | 19 | 10 |  DISPLAY_SPI_MOSI|
| 10 | 23 | 11 | DISPLAY_SPI_CLK|