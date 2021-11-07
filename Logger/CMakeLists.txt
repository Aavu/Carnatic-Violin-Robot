include_directories(
        ${CMAKE_SOURCE_DIR}/3rdparty/spdlog/Include
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
        )

if (NOT TARGET spdlog)
        find_library(spdlog NAMES libspdlog.a REQUIRED PATHS ${CMAKE_SOURCE_DIR}/3rdparty/spdlog/lib/ NO_DEFAULT_PATH)
endif()

add_library(
        ${LOGGER_LIB}
        Src/Logger.cpp)

target_include_directories(${LOGGER_LIB} PUBLIC ${CMAKE_SOURCE_DIR}/3rdparty/spdlog/Include ${CMAKE_CURRENT_SOURCE_DIR}/Include )
target_link_directories(${LOGGER_LIB} PUBLIC ${CMAKE_SOURCE_DIR}/3rdparty/spdlog/lib/)
target_link_libraries(${LOGGER_LIB} PUBLIC spdlog)

target_precompile_headers(${LOGGER_LIB}
        PUBLIC
        ${CMAKE_SOURCE_DIR}/Include/pch.h
        PRIVATE
        ${CMAKE_SOURCE_DIR}/Logger/Include/spdlog_pch.h
        )