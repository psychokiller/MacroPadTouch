idf_component_register(SRCS "MacroPadTouch.cpp"
                "touch/TouchDriver.cpp"
                "touch/GT1151.cpp"
                "touch/ICNT86X.cpp"
                "touch/TouchPoint.cpp"
                "Utils.cpp"
                "display/ePaper/Display.cpp"
                "display/ePaper/WaveShare213.cpp"
                "display/ePaper/WaveShare29.cpp"
                "fonts/font8.cpp"
                "fonts/font12.cpp"
                "fonts/font16.cpp"
                "fonts/font20.cpp"
                "fonts/font24.cpp"
                "paint/GUI_Paint.cpp"
                "graphics/Button.cpp"
                "ui/UiManager.cpp"
                "bluetooth/BleKeyboard.cpp"
                "configuration/ButtonConfig.cpp"
                "graphics/IconLoader.cpp"
                "services/AsyncWebServerManager.cpp"
                "services/ApStaConfigRouteHandlers.cpp"
                "services/FileRouteHandlers.cpp"
                "log_wrap_stub.c"
                INCLUDE_DIRS "."
            PRIV_REQUIRES esp_insights esp_diagnostics
REQUIRES driver esp_timer nvs_flash spiffs esp-nimble-cpp esp_http_server NvsConfigManager WifiManager FileManager espasyncwebserver 
                )

target_link_options(${COMPONENT_TARGET} PRIVATE
    "-Wl,--wrap=esp_log_write"
    "-Wl,--wrap=esp_log_writev"
)