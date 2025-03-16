# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Test1_BA_DIY_Power_PCB)
# Füge Pre-Build-Skript ein, um parsed_pins.h zu generieren
add_custom_target(generate_pins_header ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Generating parsed_pins.h"
    COMMAND python ${CMAKE_SOURCE_DIR}/generate_pins_header.py
    DEPENDS sdkconfig
)

# Die Header-Datei soll vor dem Erstellen des main-Ziels generiert werden
add_dependencies(${CMAKE_PROJECT_NAME}.elf generate_pins_header)
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")
include_directories("${CMAKE_BINARY_DIR}/config")
