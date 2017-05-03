cmake_minimum_required(VERSION 2.8.4)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)
set(PROJECT_NAME LaserHarpString)
project(${PROJECT_NAME})
#set(${PROJECT_NAME}_BOARD nano)
#set(${PROJECT_NAME}_PORT /dev/ttyUSB0)

#Hardware settings
#set(ARDUINO_DEFAULT_BOARD nano) # Default Board ID, when not specified
#set(ARDUINO_DEFAULT_PORT /dev/ttyUSB0) # Default Port, when not specified

set(${PROJECT_NAME}_SKETCH src/LaserHarpString/LaserHarpString.ino)
##file(GLOB SOURCES src/LaserHarpString/*.ino src/lib/*.cpp)
print_board_list()
print_programmer_list()
generate_arduino_firmware(${PROJECT_NAME})

