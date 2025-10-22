# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(MacroPadTouch)

# Enable C++ support for the project if not already enabled.
# If components use C++ sources, this ensures the build system knows to compile them.
# The 'CPP_SRCS' component is usually handled internally by ESP-IDF for C++ compilation.
# This line ensures that if 'WifiAccessPoint' explicitly declares a dependency on 'CPP_SRCS'
# (e.g., in an older component or a custom one that needs it), it's resolved.

set(EXTRA_COMPONENT_DIRS
    components/spiffs_image
    components/NvsConfigManager
    components/WifiManager
    components/FileManager
)

spiffs_create_partition_image(
            spiffs
            "${CMAKE_SOURCE_DIR}/data"
            FLASH_IN_PROJECT
)
