#include "Button.h"
#include "esp_timer.h"
#include "esp_log.h"

Button::Button(std::string label, sFONT *font, display_color fg, display_color bg)
{
    this->foreground_color = fg;
    this->background_color = bg;
    this->font = font;
    this->label = label;
}

void Button::draw()
{
    xPosEnd = xPos + height;
    yPosEnd = yPos + width;
    Paint_DrawString_EN(xPos + (font->Width / 2) - 3, yPos + (font->Height / 2) + 5, label.c_str(), font, background_color, foreground_color);
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
std::string Button::get_text()
{
    return this->label;
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