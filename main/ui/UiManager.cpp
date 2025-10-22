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
     ESP_LOGI("UI", "New Coordinates: x: %i, y: %i", display.get_width() - tp->x, tp->y);

    TouchPoint adjusted_tp = transform_touch(*tp, display.get_width(), display.get_height());
     ESP_LOGI("UI", "Adjusted Coordinates: x: %i, y: %i", adjusted_tp.x, adjusted_tp.y);
    for (Button* b : buttons) {
        if (b->isPressed(adjusted_tp.x, adjusted_tp.y)){
             ESP_LOGI("UI", "Button pressed: %s on %d, %d", b->get_label().c_str(), tp->x, tp->y);
            return b;
        }
    }

    return nullptr;
}

TouchPoint UiManager::getPressedCoordinates(TouchPoint* tp) {
    TouchPoint adjusted_tp;
    adjusted_tp.x = tp->y;
    adjusted_tp.y = this->display.get_width() - tp->x;
    adjusted_tp.size = tp->size;
    adjusted_tp.track_id = tp->track_id;
    
    return adjusted_tp;
}
