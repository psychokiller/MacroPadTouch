#pragma once

#include "ESPAsyncWebServer.h"

namespace ApStaConfigRouteHandlers {
    /**
     * @brief Sets up all WiFi configuration routes.
     */
    void setup_ap_sta_config_routes(AsyncWebServer& server);
}