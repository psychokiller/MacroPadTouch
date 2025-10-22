#ifndef _UiManager_H_
#define _UiManager_H_

#include <stdio.h>
#include "../graphics/Button.h"
#include "../display/ePaper/Display.h"
#include <vector>


class UiManager {
    public:
    UiManager(Display& display, size_t rows, size_t cols);
    ~UiManager();

    void draw(std::vector<Button*>& buttons);
    Button* getPressedButton(TouchPoint* tp, std::vector<Button*>& buttons);
    TouchPoint getPressedCoordinates(TouchPoint* tp);
    
    private:
    Display &display;
    size_t grid_rows;
    size_t grid_cols;
};

#endif