# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# set(EXTRA_COMPONENT_DIRS "LZRTag/XasCode/ESP32" )
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(SCP_MTF_DEI_PDA)
target_add_binary_data(SCP_MTF_DEI_PDA.elf "main/hivemq.pem" TEXT)