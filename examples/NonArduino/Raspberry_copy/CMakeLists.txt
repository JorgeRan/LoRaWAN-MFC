cmake_minimum_required(VERSION 3.18)

# create the project
project(rpi-sx1262)

# when using debuggers such as gdb, the following line can be used
#set(CMAKE_BUILD_TYPE Debug)

# if you did not build RadioLib as shared library (see wiki),
# you will have to add it as source directory
# the following is just an example, yours will likely be different
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../.." "${CMAKE_CURRENT_BINARY_DIR}/Radiolib")

# add the executable
add_executable(${PROJECT_NAME} main.cpp)

# link both libraries
# Use keyword form to avoid mixing plain and keyword signatures
target_link_libraries(${PROJECT_NAME} PRIVATE RadioLib lgpio wiringPi)

# Find nlohmann_json; prefer system package if available, otherwise fetch v3.12.0
# find_package(nlohmann_json QUIET)
# if (nlohmann_json_FOUND)
#   message(STATUS "Using system nlohmann_json")
#   target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
# else()
#   message(STATUS "nlohmann_json not found; fetching v3.12.0 with FetchContent")
#   include(FetchContent)
#   FetchContent_Declare(
#     nlohmann_json
#     GIT_REPOSITORY https://github.com/nlohmann/json.git
#     GIT_TAG v3.12.0
#   )
#   FetchContent_MakeAvailable(nlohmann_json)
#   target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
# endif()

# you can also specify RadioLib compile-time flags here
#target_compile_definitions(RadioLib PUBLIC RADIOLIB_DEBUG_BASIC RADIOLIB_DEBUG_SPI)
#target_compile_definitions(RadioLib PUBLIC RADIOLIB_DEBUG_PORT=stdout)
