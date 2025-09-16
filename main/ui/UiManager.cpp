#include "UiManager.h"
#include "esp_log.h"

UiManager::UiManager(Display& display, size_t rows, size_t cols) : display(display), grid_rows(rows), grid_cols(cols) {}

void UiManager::draw(std::vector<Button*> &buttons) {

   size_t max_buttons = grid_rows * grid_cols;
    size_t num_buttons = buttons.size() > max_buttons ? max_buttons : buttons.size();

    uint16_t btn_w = display.get_height() / grid_cols;
    uint16_t btn_h = display.get_width() / grid_rows;

    for (size_t i = 0; i < num_buttons; ++i) {
        Button* button = buttons[i];
        size_t row = i / grid_cols;
        size_t col = i % grid_cols;

        uint16_t x = col * btn_w;
        uint16_t y = row * btn_h;

        button->set_width(btn_h);
        button->set_height(btn_w);
        button->set_xPos(x);
        button->set_yPos(y);

        button->draw();
        ESP_LOGI("UI", "Btn Header: %s, Btn XPos %d, yPos: %d, xEnd: %d, yEnd: %d", button->get_text().c_str(), button->get_xPos(), button->get_yPos(), button->get_xPosEnd(), button->get_yPosEnd());
    }
}

bool UiManager::isButtonPressed(TouchPoint *tp, std::vector<Button*>& buttons)
{   
     ESP_LOGI("UI", "Display dimensions: h: %i, w: %i", display.get_height(), display.get_width());
     ESP_LOGI("UI", "New Coordinates: x: %i, y: %i", display.get_width() - tp->x, tp->y);

     uint16_t tmp = tp->x;
    tp->x = tp->y;
     tp->y = this->display.get_width() - tmp;
    for (Button* b : buttons) {
        if (b->isPressed(tp->x, tp->y)){
             ESP_LOGI("UI", "Button pressed: %s on %d, %d", b->get_text().c_str(), tp->x, tp->y);
            return true;
        }
    }

    return false;
}
