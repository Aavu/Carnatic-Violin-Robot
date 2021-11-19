add_library(
        Dynamixel
        Src/Dynamixel.cpp)

target_include_directories(Dynamixel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_include_directories(Dynamixel PUBLIC /usr/local/include/dynamixel_sdk)
target_link_libraries(Dynamixel PUBLIC dxl_sbc_cpp)