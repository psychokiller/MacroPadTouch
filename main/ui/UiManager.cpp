#include "UiManager.h"
#include "esp_log.h"

UiManager::UiManager(Display& display, size_t rows, size_t cols) : display(display), grid_rows(rows), grid_cols(cols) {}

void UiManager::draw(std::vector<Button*> &buttons) {

   size_t max_buttons = grid_rows * grid_cols;
    size_t num_buttons = buttons.size() > max_buttons ? max_buttons : buttons.size();

    for (size_t i = 0; i < num_buttons; ++i) {
        Button* button = buttons[i];
        Rect r = compute_grid_cell(display.get_width(), display.get_height(), grid_rows, grid_cols, i);

        // UiManager historically set height = btn_w and width = btn_h — keep that behavior
        uint16_t btn_w = display.get_height() / grid_cols;
        uint16_t btn_h = display.get_width() / grid_rows;

        button->set_width(btn_h);
        button->set_height(btn_w);
        button->set_xPos(r.x);
        button->set_yPos(r.y);
        button->set_index(i+1);

        button->draw();
        ESP_LOGI("UI", "Btn Header: %s, Btn XPos %d, yPos: %d, xEnd: %d, yEnd: %d", button->get_label().c_str(), button->get_xPos(), button->get_yPos(), button->get_xPosEnd(), button->get_yPosEnd());
    }
}

Button* UiManager::getPressedButton(TouchPoint *tp, std::vector<Button*>& buttons)
{   
     ESP_LOGI("UI", "Display dimensions: h: %i, w: %i", display.get_height(), display.get_width());
     ESP_LOGI("UI", "Touch Coordinates: x: %i, y: %i", tp->x, tp->y);

    for (Button* b : buttons) {
        if (b->isPressed(tp->x, tp->y)){
             ESP_LOGI("UI", "Button pressed: %s on %d, %d", b->get_label().c_str(), tp->x, tp->y);
            return b;
        }
    }

    return nullptr;
}

TouchPoint UiManager::getPressedCoordinates(TouchPoint* tp) {
    // Coordinates are already transformed by the touch driver
    return *tp;
}

Rect UiManager::compute_grid_cell(uint16_t display_width, uint16_t display_height, size_t rows, size_t cols, size_t index) {
    Rect r{0,0,0,0};
    uint16_t btn_w = display_height / cols; // width per column
    uint16_t btn_h = display_width / rows;  // height per row

    size_t row = index / cols;
    size_t col = index % cols;

    uint16_t x = col * btn_w;
    uint16_t y = row * btn_h;

    r.x = x;
    r.y = y;
    r.xEnd = x + btn_w; // xEnd = x + width assigned to button height in UiManager
    r.yEnd = y + btn_h;
    return r;
}