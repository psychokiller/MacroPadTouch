#ifndef _UiManager_H_
#define _UiManager_H_

#include <stdio.h>
#include "../graphics/Button.h"
#include "../display/ePaper/Display.h"
#include <vector>


class UiManager {
    public:
    UiManager(Display& display);
    ~UiManager();

    void draw(std::vector<Button*>& buttons);
    
    private:
    
    uint16_t number_of_rows;
    uint16_t btn_height;
    uint16_t btn_width;
    static const uint16_t number_of_cols = 3;
    static const uint16_t total_number_of_buttons = 6;
    static const uint16_t btn_spacer = 2;
};

#endif