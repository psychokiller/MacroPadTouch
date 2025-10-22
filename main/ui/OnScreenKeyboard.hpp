#include "paint/GUI_Paint.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_log.h"

// --- Configuration ---
#define MAX_PASSWORD_LENGTH 64
#define SCREEN_WIDTH 250
#define SCREEN_HEIGHT 122
#define KEY_HEIGHT 20
#define KEY_SPACING 3
#define DISPLAY_HEIGHT 25
#define KB_START_Y (DISPLAY_HEIGHT + 5) // Start keyboard below display area

// Assuming display colors are defined in GUI_Paint.h
#define COLOR_KEY_DEFAULT WHITE
#define COLOR_KEY_TEXT BLACK
#define COLOR_KEY_ACTIVE BLACK
#define COLOR_KEY_ACTIVE_TEXT WHITE

// --- Data Structures ---

typedef enum {
    KEY_TYPE_CHAR,
    KEY_TYPE_SHIFT,
    KEY_TYPE_BACKSPACE,
    KEY_TYPE_SPACE,
    KEY_TYPE_ENTER,
    KEY_TYPE_NONE // Used for padding/unassigned space
} KeyType_t;

typedef enum {
    KEY_MODE_ALPHA,
    KEY_MODE_NUMERIC
} KeyboardMode_t;

typedef struct {
    uint16_t x_start;
    uint16_t y_start;
    uint16_t x_end;
    uint16_t y_end;
    KeyType_t type; // The field that caused the error!
    char primary_char; // Lowercase/un-shifted char or number/symbol
    char shifted_char; // Uppercase/shifted symbol (if applicable)
    const char* label; // Display label (e.g., "Shift", "123", "⌫")
    float width_ratio; // Width relative to a standard key (e.g., 1.0, 2.5)
} Key_t;


typedef struct {
    char password[MAX_PASSWORD_LENGTH + 1];
    uint8_t length;
    bool is_shifted;
} KeyboardState_t;

// --- Global State ---
static KeyboardState_t g_kb_state = {
    .password = {0},
    .length = 0,
    .is_shifted = false
};

static KeyboardMode_t g_kb_mode = KEY_MODE_ALPHA; // Start in letters mode

// --- Function Prototypes ---
static void apply_current_keymap(void);
static void draw_key(const Key_t* key, bool is_active);
static void draw_password_display();
static void toggle_shift(void);
void Keyboard_Draw(void); // Defined later

// --- Key Layout Definition ---
#define STANDARD_KEY_RATIO 1.0f
#define TOTAL_KEYS 33 // 10 (Row 1) + 9 (Row 2) + 9 (Row 3) + 5 (Control)

// A modifiable copy of the key definitions (stores geometry and current state)
static Key_t g_keys[TOTAL_KEYS]; 

// Geometry template (contains type, ratio, and fixed labels for control keys)
static const Key_t KEY_DEFS_GEOMETRY_TEMPLATE[TOTAL_KEYS] = {
    // Row 1 (10 keys) - Defined as KEY_TYPE_CHAR in the template
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, // 9

    // Row 2 (9 keys, slightly offset) - Defined as KEY_TYPE_CHAR in the template
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, // 18

    // Row 3 (9 keys) - Defined as KEY_TYPE_CHAR in the template
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, NULL, 1.0f}, // 27

    // Row 4 (Control Row - uses explicit control types)
    {0, 0, 0, 0, KEY_TYPE_SHIFT, 0, 0, "", 1.5f},      // 28
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, "", 1.5f},         // 29 - MODE SWITCH KEY (treated as CHAR for touch)
    {0, 0, 0, 0, KEY_TYPE_SPACE, ' ', ' ', "Space", 4.0f},   // 30
    {0, 0, 0, 0, KEY_TYPE_BACKSPACE, 0, 0, "", 1.5f},      // 31
    {0, 0, 0, 0, KEY_TYPE_ENTER, 0, 0, "", 1.5f}       // 32
};

// Character map for Alpha mode (Only the first 28 slots used for dynamic char content)
static const Key_t KEY_MAP_ALPHA[TOTAL_KEYS] = {
    // R1: Q W E R T Y U I O P
    {0, 0, 0, 0, KEY_TYPE_CHAR, 'q', 'Q', "q", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'w', 'W', "w", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'e', 'E', "e", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 'r', 'R', "r", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 't', 'T', "t", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'y', 'Y', "y", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 'u', 'U', "u", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'i', 'I', "i", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'o', 'O', "o", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 'p', 'P', "p", 0.0f}, // 9
    
    // R2: A S D F G H J K L
    {0, 0, 0, 0, KEY_TYPE_CHAR, 'a', 'A', "a", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 's', 'S', "s", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'd', 'D', "d", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 'f', 'F', "f", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'g', 'G', "g", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'h', 'H', "h", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 'j', 'J', "j", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'k', 'K', "k", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'l', 'L', "l", 0.0f}, // 18

    // R3: Z X C V B N M . ,
    {0, 0, 0, 0, KEY_TYPE_CHAR, 'z', 'Z', "z", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'x', 'X', "x", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'c', 'C', "c", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 'v', 'V', "v", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'b', 'B', "b", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, 'n', 'N', "n", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, 'm', 'M', "m", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '.', '>', ".", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, ',', '<', ",", 0.0f}, // 27
    
    // R4 (Control keys use explicit types)
    {0, 0, 0, 0, KEY_TYPE_SHIFT, 0, 0, "Shift", 1.5f},       // 28
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, "123", 1.5f},        // 29 - Mode switch
    {0, 0, 0, 0, KEY_TYPE_SPACE, ' ', ' ', "Space", 4.0f},   // 30
    {0, 0, 0, 0, KEY_TYPE_BACKSPACE, 0, 0, "BSP", 1.5f},   // 31
    {0, 0, 0, 0, KEY_TYPE_ENTER, 0, 0, "ENTER", 1.5f},       // 32
};

// Character map for Numeric mode
static const Key_t KEY_MAP_NUMERIC[TOTAL_KEYS] = {
    // R1: 1 2 3 4 5 6 7 8 9 0
    {0, 0, 0, 0, KEY_TYPE_CHAR, '1', '!', "1", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '2', '@', "2", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '3', '#', "3", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, '4', '$', "4", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '5', '%', "5", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '6', '^', "6", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, '7', '&', "7", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '8', '*', "8", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '9', '(', "9", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, '0', ')', "0", 0.0f}, // 9

    // R2: - / : ; ( ) $ & @
    {0, 0, 0, 0, KEY_TYPE_CHAR, '-', '_', "-", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '/', '?', "/", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, ':', '{', ":", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, ';', '}', ";", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '(', '[', "(", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, ')', ']', ")", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, '$', '~', "$", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '&', '|', "&", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '@', '\\', "@", 0.0f}, // 18

    // R3: # % ^ * + = _ < >
    {0, 0, 0, 0, KEY_TYPE_CHAR, '#', '`', "#", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '%', '"', "%", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '^', '\'', "^", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, '*', '=', "*", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '+', '<', "+", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '=', '>', "=", 0.0f}, 
    {0, 0, 0, 0, KEY_TYPE_CHAR, '_', '!', "_", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '<', '~', "<", 0.0f}, {0, 0, 0, 0, KEY_TYPE_CHAR, '>', '`', ">", 0.0f}, // 27
    
    // R4 (Control keys use explicit types)
    {0, 0, 0, 0, KEY_TYPE_SHIFT, 0, 0, "Shift", 1.5f},       // 28
    {0, 0, 0, 0, KEY_TYPE_CHAR, 0, 0, "ABC", 1.5f},        // 29 - Mode switch
    {0, 0, 0, 0, KEY_TYPE_SPACE, ' ', ' ', "Space", 4.0f},   // 30
    {0, 0, 0, 0, KEY_TYPE_BACKSPACE, 0, 0, "BSP", 1.5f},   // 31
    {0, 0, 0, 0, KEY_TYPE_ENTER, 0, 0, "ENTER", 1.5f},       // 32
};


/**
 * @brief Recalculates the position (x_start, x_end) for all keys based on screen size.
 * Must be called once before drawing to initialize geometry.
 */
static void calculate_key_positions(void) {
    // 1. Copy the template to g_keys (geometry, type, fixed labels)
    memcpy(g_keys, KEY_DEFS_GEOMETRY_TEMPLATE, sizeof(KEY_DEFS_GEOMETRY_TEMPLATE));

    uint16_t current_y = KB_START_Y;
    
    // We assume the widest row (Row 1) defines the key sizing: 10 units.
    float row_unit_sum = 10.0f; 
    
    // Total width available for keys in a row, minus padding
    uint16_t total_spacing = (uint16_t)((row_unit_sum + 1) * KEY_SPACING); // 11 spaces total
    uint16_t usable_width = SCREEN_WIDTH - total_spacing; 
    
    // The width of a 1.0 ratio key
    uint16_t unit_width = (uint16_t)(usable_width / row_unit_sum);

    // The key calculation must run row by row
    uint8_t key_index = 0;
    // Keys per row: 10 (QWERTY), 9 (ASDF), 9 (ZXCV), 5 (Control)
    const uint8_t keys_per_row[] = {10, 9, 9, 5}; 
    // Offset in units of (unit_width + KEY_SPACING) to center rows
    const float row_offsets[] = {0.2f, 0.7f, 0.2f, 0.0f}; 

    for (int row = 0; row < 4; row++) { // Now only 4 rows
        // Calculate the starting X position with the row offset
        uint16_t row_start_offset = (uint16_t)(row_offsets[row] * (unit_width + KEY_SPACING));
        uint16_t current_x = KEY_SPACING + row_start_offset;
        
        for (int i = 0; i < keys_per_row[row]; i++) {
            Key_t* key = &g_keys[key_index];
            
            // Calculate key width based on its ratio
            uint16_t key_width = (uint16_t)(key->width_ratio * unit_width) + 
                                 (uint16_t)((key->width_ratio - 1) * KEY_SPACING);

            key->x_start = current_x;
            key->y_start = current_y;
            key->x_end = current_x + key_width;
            key->y_end = current_y + KEY_HEIGHT;
            
            current_x = key->x_end + KEY_SPACING;
            key_index++;
        }
        current_y += KEY_HEIGHT + KEY_SPACING;
    }
}


/**
 * @brief Loads the character and label data based on the current keyboard mode.
 */
static void apply_current_keymap(void) {
    const Key_t *map = (g_kb_mode == KEY_MODE_ALPHA) ? KEY_MAP_ALPHA : KEY_MAP_NUMERIC;

    for (size_t i = 0; i < TOTAL_KEYS; i++) {
        // Apply character data for all keys
        g_keys[i].primary_char = map[i].primary_char;
        g_keys[i].shifted_char = map[i].shifted_char;
        
        // Update labels for dynamic character keys where label is char/symbol
        if (g_keys[i].type == KEY_TYPE_CHAR || g_keys[i].type == KEY_TYPE_SPACE) {
            if (map[i].label != NULL) {
                g_keys[i].label = map[i].label;
            }
        }

        // For the mode switch key (index 29), we update the label to reflect the *next* mode.
        if (i == 29) {
            g_keys[i].label = (g_kb_mode == KEY_MODE_ALPHA) ? "123" : "ABC";
        }
    }
}

/**
 * @brief Draws a single key.
 */
static void draw_key(const Key_t* key, bool is_active) {
    display_color bg_color = is_active ? COLOR_KEY_ACTIVE : COLOR_KEY_DEFAULT;
    display_color fg_color = is_active ? COLOR_KEY_ACTIVE_TEXT : COLOR_KEY_TEXT;
    
    // The pointer to the string we will pass to Paint_DrawString_EN
    const char *label_to_draw = key->label; 
    
    // Safe buffer for single characters (to avoid dangling pointers)
    char char_buffer[2] = {0, 0}; 

    // Logic for dynamic character keys (letters, numbers, symbols)
    if (key->type == KEY_TYPE_CHAR || key->type == KEY_TYPE_SPACE) {
        // If label is NULL, it means it's a single-character key and we need to use the char fields.
        if (key->label == NULL) { 
            char display_char;

            if (g_kb_state.is_shifted) {
                // Use shifted character if available, otherwise primary char
                display_char = (key->shifted_char != 0) ? key->shifted_char : key->primary_char;
            } else {
                // Use primary character
                display_char = key->primary_char;
            }
            
            char_buffer[0] = display_char;
            label_to_draw = char_buffer; // Point to the safe, local buffer
        }
    } 
    
    // Special case for Shift key to show active state label
    if (key->type == KEY_TYPE_SHIFT && g_kb_state.is_shifted) {
        bg_color = COLOR_KEY_ACTIVE; // Active background
        fg_color = COLOR_KEY_ACTIVE_TEXT; // Active text
    }

    // 1. Draw key background
    Paint_DrawRectangle(key->x_start, key->y_start, key->x_end, key->y_end, 
                        bg_color, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // 2. Draw key border
    Paint_DrawRectangle(key->x_start, key->y_start, key->x_end, key->y_end, 
                        COLOR_KEY_TEXT, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    
    // 3. Draw key label (centered)
    
    // Calculate text position for centering (using Font16 width/height)
    // NOTE: strlen(label_to_draw) * Font16.Width is used here
    uint16_t text_width = strlen(label_to_draw) * Font16.Width; 
    uint16_t x_center = key->x_start + (key->x_end - key->x_start) / 2;
    uint16_t y_center = key->y_start + (key->y_end - key->y_start) / 2;
    
    // Use Font16 for key labels
    Paint_DrawString_EN(x_center - (text_width / 2), 
                        y_center - (Font16.Height / 2),
                        label_to_draw, 
                        &Font16, fg_color, bg_color);
}


/**
 * @brief Draws the masked password display field.
 */
static void draw_password_display() {
    // 1. Clear the display area
    Paint_DrawRectangle(0, 0, SCREEN_WIDTH, DISPLAY_HEIGHT, 
                        WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawRectangle(0, 0, SCREEN_WIDTH, DISPLAY_HEIGHT, 
                        BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    
    // 2. Draw the masked password (using '*')
    char masked_buffer[MAX_PASSWORD_LENGTH + 1];
    memset(masked_buffer, '*', g_kb_state.length);
    masked_buffer[g_kb_state.length] = '\0';

    // Use Font24 for the display area
    Paint_DrawString_EN(KEY_SPACING, 
                        (DISPLAY_HEIGHT / 2) - (Font24.Height / 2),
                        masked_buffer,
                        &Font24, BLACK, WHITE);
    
    // 3. Draw cursor (small vertical line at the end)
    if (g_kb_state.length < MAX_PASSWORD_LENGTH) {
        uint16_t cursor_x = KEY_SPACING + g_kb_state.length * Font24.Width + 2;
        Paint_DrawLine(cursor_x, 
                       (DISPLAY_HEIGHT / 2) - (Font24.Height / 2) + 1, 
                       cursor_x, 
                       (DISPLAY_HEIGHT / 2) + (Font24.Height / 2) - 1, 
                       BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }
}


/**
 * @brief Toggles the shift state and redraws relevant keys.
 */
static void toggle_shift(void) {
    g_kb_state.is_shifted = !g_kb_state.is_shifted;
    
    // Redraw all keys, as shift affects many labels and the shift key's color
    for (size_t i = 0; i < TOTAL_KEYS; i++) {
        draw_key(&g_keys[i], false); // Redraw with new state
    }
}


/**
 * @brief Public function to initialize and draw the entire keyboard interface.
 */
void Keyboard_Draw(void) {
    // 1. Calculate positions (needed only once)
    static bool geometry_calculated = false;
    if (!geometry_calculated) {
        calculate_key_positions();
        geometry_calculated = true;
    }
    
    // 2. Apply the current mode's characters
    apply_current_keymap();

    // 3. Draw the password display
    draw_password_display();

    // 4. Draw all keys
    for (size_t i = 0; i < TOTAL_KEYS; i++) {
        draw_key(&g_keys[i], false); // is_active=false for initial draw
    }
}


/**
 * @brief Public function to handle a simulated touch event.
 * @param x_touch X coordinate of the touch/click.
 * @param y_touch Y coordinate of the touch/click.
 * @return True if the password is ready (Enter pressed), False otherwise.
 */
bool Keyboard_HandleTouch(uint16_t x_touch, uint16_t y_touch) {
    
    for (size_t i = 0; i < TOTAL_KEYS; i++) {
        const Key_t* key = &g_keys[i];

        // Check if the touch falls within the key boundaries
        if (x_touch >= key->x_start && x_touch <= key->x_end &&
            y_touch >= key->y_start && y_touch <= key->y_end) {

            // Simulate key press visual feedback (Draw active state)
            // draw_key(key, true); 
            ESP_LOGI("OnScreenKeyboard", "Keyboard key pressed: %c", key->primary_char );
            // Handle the key logic
            switch (key->type) {
                case KEY_TYPE_CHAR: {
                    
                    break;
                }
                case KEY_TYPE_SPACE: {
                    // Check for the mode switch key (index 29)
                    if (i == 29) {
                        g_kb_mode = (g_kb_mode == KEY_MODE_ALPHA) ? KEY_MODE_NUMERIC : KEY_MODE_ALPHA;
                        // Since Keyboard_Draw calls apply_current_keymap(), it updates the labels automatically.
                        Keyboard_Draw(); // Full redraw for mode change
                        break;
                    }
                    
                    // Handle standard character input
                    if (g_kb_state.length < MAX_PASSWORD_LENGTH) {
                        char char_to_add;
                        if (g_kb_state.is_shifted) {
                            // If key->label is NULL (it's a single char key), use the shifted char field
                            if (key->label == NULL) {
                                char_to_add = (key->shifted_char != 0) ? key->shifted_char : key->primary_char;
                            } else {
                                // For keys like ".", "-", etc., use the label content
                                char_to_add = (key->shifted_char != 0) ? key->shifted_char : key->primary_char;
                            }
                        } else {
                            char_to_add = key->primary_char;
                        }
                        
                        g_kb_state.password[g_kb_state.length] = char_to_add;
                        g_kb_state.length++;
                        g_kb_state.password[g_kb_state.length] = '\0';
                        
                        // If shift was active for a single character, toggle it off
                        if (g_kb_state.is_shifted) {
                            toggle_shift(); 
                        }
                        draw_password_display();
                    }
                    break;
                }
                case KEY_TYPE_SHIFT: {
                    toggle_shift();
                    break;
                }
                case KEY_TYPE_BACKSPACE: {
                    if (g_kb_state.length > 0) {
                        g_kb_state.length--;
                        g_kb_state.password[g_kb_state.length] = '\0';
                        draw_password_display();
                    }
                    break;
                }
                case KEY_TYPE_ENTER: {
                    if (g_kb_state.length >= 8) {
                        // Success: Password ready
                        return true; 
                    }
                    // TODO: Implement visual feedback for password too short
                    break;
                }
                default:
                    break;
            }
            
            // Redraw key back to default state (after handling logic)
            draw_key(key, false); 
            
            return false; // Not finished entering password
        }
    }
    return false; // No key pressed
}

/**
 * @brief Retrieves the completed password string.
 */
const char* Keyboard_GetPassword(void) {
    return g_kb_state.password;
}

/**
 * @brief Resets the keyboard state.
 */
void Keyboard_Reset(void) {
    g_kb_state.length = 0;
    g_kb_state.password[0] = '\0';
    g_kb_state.is_shifted = false;
    g_kb_mode = KEY_MODE_ALPHA;
}
