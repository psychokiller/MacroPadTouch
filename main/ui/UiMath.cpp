#include "UiMath.h"

Rect compute_grid_cell(uint16_t display_width, uint16_t display_height, size_t rows, size_t cols, size_t index) {
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

TouchPoint transform_touch(const TouchPoint &tp, uint16_t display_width, uint16_t display_height) {
    TouchPoint adjusted;
    adjusted.x = tp.y;
    adjusted.y = display_width - tp.x;
    adjusted.size = tp.size;
    adjusted.track_id = tp.track_id;
    return adjusted;
}
