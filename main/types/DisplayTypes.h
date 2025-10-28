#ifndef _DISPLAY_TYPES_H_
#define _DISPLAY_TYPES_H_

// Common display-related types used across components
typedef enum {
    MIRROR_NONE  = 0x00,
    MIRROR_HORIZONTAL = 0x01,
    MIRROR_VERTICAL = 0x02,
    MIRROR_ORIGIN = 0x03,
} MIRROR_IMAGE;

#endif