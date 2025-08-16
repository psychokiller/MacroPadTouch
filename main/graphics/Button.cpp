#include "Button.h"

Button::Button(std::string text, sFONT* font, display_color fg, display_color bg) {
    this->foreground_color = fg;
    this->background_color = bg;
    this->font = font;
    this->text = text;
}

void Button::draw() {
    Paint_DrawString_EN(xPos + (font->Width / 2) - 3, yPos + (font->Height / 2) + 5, text.c_str(), font, background_color, foreground_color);
    Paint_DrawRectangle(xPos, yPos , xPos + height, yPos +  width, foreground_color, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
}

void Button::set_height(uint16_t h) {
    this->height = h;
}

void Button::set_width(uint16_t w) {
    this->width = w;
}

void Button::set_xPos(uint16_t x) {
    this->xPos = x;
}

void Button::set_yPos(uint16_t y) {
    this->yPos = y;
}