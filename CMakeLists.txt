cmake_minimum_required(VERSION 3.13)
project (Hathaani)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RE LEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LOGGER_LIB Logger)
set(JSON_LIB rapidjson)

include_directories(
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/3rdparty/spdlog/Include
        ${CMAKE_SOURCE_DIR}/3rdparty/${JSON_LIB}/Include
        ${CMAKE_SOURCE_DIR}/${LOGGER_LIB}/Include
)

add_subdirectory(${LOGGER_LIB})
add_subdirectory(Dynamixel)
add_subdirectory(${PROJECT_NAME})
add_subdirectory(Tuner)