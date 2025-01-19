#ifndef _TouchPoint_H_
#define _TouchPoint_H_

#include <stdio.h>

// Basic structure to hold a point data
// based on the GT1151N Data sheet
// But likely similar to the ICNT86X
class TouchPoint
{
public:
    TouchPoint();
    ~TouchPoint();
    uint8_t track_id; // the trackId of the touch point
    uint8_t x;        // x-axis value/coordinate
    uint8_t y;        // y-axis value/coordinate
    uint8_t size;     // size of the touch point
};

#endif