cmake_minimum_required(VERSION 3.13)
project (CarnaticViolinist)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(Violinist)
add_subdirectory(Tuner)

#include_directories(${CMAKE_SOURCE_DIR}/include)

#set(PitchFileParserLIB PitchFileParser)
#set(TunerLIB Tuner)
#add_subdirectory(src)
#add_subdirectory(${TunerLIB})

#add_library(${PitchFileParserLIB} include/${PitchFileParserLIB}.h src/${PitchFileParserLIB}.cpp)

#target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
#target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src)

#target_link_libraries(${PROJECT_NAME} -lEposCmd -lpthread)