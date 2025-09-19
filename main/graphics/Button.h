#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdio.h>
#include <string>
#include "esp_system.h"
#include "../paint/GUI_Paint.h"
#include "../fonts/fonts.h"
#include "../touch/TouchPoint.h"

class Button {
    public:

        Button(std::string label, sFONT* font, display_color fg, display_color bg);
        ~Button();
        void set_width(uint16_t width);
        void set_height(uint16_t height);
        void set_xPos(uint16_t xPos);
        void set_yPos(uint16_t yPos);
        
        void draw();

        bool isPressed(uint16_t x, uint16_t y);

        std::string get_label();
        uint16_t get_xPos() { return xPos; }
        uint16_t get_yPos() { return yPos; }
        uint16_t get_xPosEnd() { return xPosEnd; }
        uint16_t get_yPosEnd() { return yPosEnd; }
        uint8_t get_index() { return index; }
        void set_index(uint8_t idx) { index = idx; }
    
    private:    
        uint16_t width;
        uint16_t height;
        uint16_t xPos;
        uint16_t yPos;
        uint16_t xPosEnd;
        uint16_t yPosEnd;
        uint8_t index;
        std::string label;
        display_color foreground_color;
        display_color background_color;
        sFONT* font;
        uint64_t lastPressTime;
        bool isBtnPressed = false;
};

#endif
