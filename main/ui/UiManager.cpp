#include "UiManager.h"
#include "esp_log.h"

UiManager::UiManager(Display& display) {
    // all calculations are in Portrait orientation
    this->number_of_rows = total_number_of_buttons / number_of_cols;
    this->btn_width = (display.get_width() / number_of_rows) - (5 * btn_spacer);
    this->btn_height = (display.get_height() / number_of_cols) - (2 * btn_spacer);
}

void UiManager::draw(std::vector<Button*> &buttons) {
    uint16_t j = 0;

    for (size_t i = 0; i < buttons.size(); i++)
    {
        Button* button = buttons[i];

        button->set_height(btn_height);
        button->set_width(btn_width);
        
        if (i >= number_of_cols) {
            button->set_xPos( (j * btn_height) + ((j+1) * 2 * btn_spacer));
            button->set_yPos(btn_width + (4 * btn_spacer));
            j++;
        } else {
            button->set_xPos( (i * btn_height) + ((i+1) * 2 * btn_spacer));
            button->set_yPos(2 * btn_spacer);
        }
        
        // ESP_LOGI("UI", "Btn Header: %s, Btn XPos %d, yPos: %d", button->text.c_str(), button->xPos, button->yPos);
        
        button->draw();
    }
    
}