#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdio.h>
#include <string>
#include "esp_system.h"
#include "../paint/GUI_Paint.h"
#include "../fonts/fonts.h"

class Button {
    public:

        Button(std::string text, sFONT* font, display_color fg, display_color bg);
        ~Button();
        void set_width(uint16_t width);
        void set_height(uint16_t height);
        void set_xPos(uint16_t xPos);
        void set_yPos(uint16_t yPos);
        
        void draw();
    
    private:    
        uint16_t width;
        uint16_t height;
        uint16_t xPos;
        uint16_t yPos;
        std::string text;
        display_color foreground_color;
        display_color background_color;
        sFONT* font;
        
};

#endif
