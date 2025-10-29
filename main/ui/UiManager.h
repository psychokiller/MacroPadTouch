#ifndef _UiManager_H_
#define _UiManager_H_

#include <stdio.h>
#include "../graphics/Button.h"
#include "../display/ePaper/Display.h"
#include "../touch/TouchDriver.h"
#include <vector>

struct Rect
{
    uint16_t x;
    uint16_t y;
    uint16_t xEnd;
    uint16_t yEnd;
};

class UiManager
{
public:
    UiManager(Display &display, size_t rows, size_t cols, TouchDriver &touch_driver);
    ~UiManager();

    void draw(std::vector<Button *> &buttons);
    Button *getPressedButton(TouchPoint *tp, std::vector<Button *> &buttons);

private:
    Display &display;
    size_t grid_rows;
    size_t grid_cols;
    TouchDriver &touch_driver;

    // Compute cell width/height and return rect for given grid cell.
    Rect compute_grid_cell(uint16_t display_width, uint16_t display_height, size_t rows, size_t cols, size_t index);
};

#endif