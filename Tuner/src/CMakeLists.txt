include_directories(${CMAKE_SOURCE_DIR}/Tuner/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
add_library(Tuner Tuner.cpp Fft.cpp rvfft.cpp)
target_link_libraries(Tuner -lasound)