cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(TestOnnx LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

include(Version)
include(Settings)
include(GNUInstallDirs)

set_global_variables()

add_executable(TestOnnxExe main.cpp)

# adding NeoML, NeoMathEngine and NeoOnnx
target_link_libraries(TestOnnxExe PRIVATE NeoML NeoOnnx NeoMathEngine)
message ("TestOnnx Prepared")