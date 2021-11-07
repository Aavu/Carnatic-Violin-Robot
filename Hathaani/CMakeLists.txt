include_directories(
        ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Include
        ${CMAKE_SOURCE_DIR}/Tuner/Include
        ${CMAKE_SOURCE_DIR}/Include
)


set(SOURCES
        src/main.cpp
        src/BowController.cpp
        src/FingerController.cpp
        src/PitchFileParser.cpp
        src/Hathaani.cpp
        src/EposController.cpp
        )


add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/Include)
target_precompile_headers(${PROJECT_NAME} REUSE_FROM ${LOGGER_LIB})
target_link_libraries(${PROJECT_NAME} Tuner -lpthread ${LOGGER_LIB})

IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    target_link_libraries(${PROJECT_NAME} -lEposCmd -lbcm2835)
    # ELSE(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")

ENDIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")