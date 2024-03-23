include_directories(${CMAKE_SOURCE_DIR}/Tuner/Include)
include_directories(${CMAKE_SOURCE_DIR}/Include)
add_library(Tuner Tuner.cpp Fft.cpp rvfft.cpp)
target_link_libraries(Tuner -lasound)