cmake_minimum_required(VERSION 2.8.12)
project (Violinist)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main.cpp Violinist.cpp Violinist.h MyDefinitions.h FingerController.cpp FingerController.h ErrorDef.h BowController.cpp BowController.h CommHandler.cpp CommHandler.h PitchFileParser.cpp PitchFileParser.h)

target_link_libraries(${PROJECT_NAME} -lEposCmd -lpthread)