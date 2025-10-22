// app_events.hpp
#pragma once
#include "esp_event.h"

// Define a new event base for application-specific events
ESP_EVENT_DECLARE_BASE(APP_EVENTS);

// Define the events
enum {
    APP_EVENT_WIFI_STA_CONNECTED,   // STA mode has successfully connected and got IP
    APP_EVENT_WIFI_AP_STARTED,      // AP mode has successfully started
    APP_EVENT_WIFI_DISCONNECTED     // STA mode disconnected
};