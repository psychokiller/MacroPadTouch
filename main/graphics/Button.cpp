#include "Button.h"
#include "esp_timer.h"
#include "esp_log.h"

Button::Button(std::string label, sFONT* font, display_color fg, display_color bg, const unsigned char* icon, uint16_t modifier, uint16_t keycode) :
    // Initialization of members that are NOT constructor args should go first (e.g., coordinates, size)
    width(0),                  // 1
    height(0),                 // 2
    xPos(0),                   // 3
    yPos(0),                   // 4
    xPosEnd(0),                // 5
    yPosEnd(0),                // 6
    index(0),                  // 7
    
    // Now initialize members that ARE constructor arguments, IN DECLARATION ORDER:
    label(label),              // 8
    foreground_color(fg),      // 9
    background_color(bg),      // 10
    icon(icon),                // 11
    modifier(modifier),        // 12
    keycode(keycode),          // 13
    font(font),                // 14
    
    // Initialize remaining members
    lastPressTime(0),          // 15
    isBtnPressed(false)        // 16
{
    // The body can be empty or contain other setup logic
}

void Button::draw()
{
    xPosEnd = xPos + height;
    yPosEnd = yPos + width;
    Paint_DrawString_EN(xPos + (font->Width / 2) - 3, yPos + (font->Height / 2) + 5, label.c_str(), font, background_color, foreground_color);
    if (icon != nullptr)
    {
        Paint_DrawImage(icon, yPos + 21 , xPos + 30, 32, 32);
    }
    Paint_DrawRectangle(xPos, yPos, xPosEnd, yPosEnd, foreground_color, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
}

void Button::set_height(uint16_t h)
{
    this->height = h;
}

void Button::set_width(uint16_t w)
{
    this->width = w;
}

void Button::set_xPos(uint16_t x)
{
    this->xPos = x;
}

void Button::set_yPos(uint16_t y)
{
    this->yPos = y;
}
std::string Button::get_label()
{
    return this->label;
}

void Button::set_modifier(uint16_t modifier) {
    this->modifier = modifier;
}

void Button::set_keycode(uint16_t keycode) {
    this->keycode = keycode;
}

bool Button::isPressed(uint16_t touch_x, uint16_t touch_y)
{
    ESP_LOGI("Button", "isPressed: %s TCoordinates: x: %i y: %i, BCoords: x: %d, y: %d, xEnd: %d, yEnd: %d", label.c_str(), touch_x, touch_y, xPos, yPos, xPosEnd, yPosEnd);

    if ((touch_x >= xPos && touch_x <= (xPosEnd) &&
         touch_y >= yPos && touch_y <= (yPosEnd)))
    {
        ESP_LOGI("Button", "Coordinates: %i %i", touch_x, touch_y);
        return true;
    }
    else
        return false;
}