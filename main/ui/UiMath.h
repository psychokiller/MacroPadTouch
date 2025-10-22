#pragma once
#include "../touch/TouchPoint.h"
#include <cstdint>

struct Rect { uint16_t x; uint16_t y; uint16_t xEnd; uint16_t yEnd; };

// Compute cell width/height and return rect for given grid cell.
Rect compute_grid_cell(uint16_t display_width, uint16_t display_height, size_t rows, size_t cols, size_t index);

// Transform touch coordinates to display coordinates (UiManager's transform)
TouchPoint transform_touch(const TouchPoint &tp, uint16_t display_width, uint16_t display_height);
