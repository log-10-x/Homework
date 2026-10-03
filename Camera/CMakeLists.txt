cmake_minimum_required(VERSION 3.10)
project(Demo1)

set(CMAKE_CXX_STANDARD 11)

set(OpenCV_DIR "/usr/lib/x86_64-linux-gnu/cmake/opencv4")
find_package(OpenCV REQUIRED)

message(STATUS "OpenCV 版本: ${OpenCV_VERSION}")
message(STATUS "OpenCV 头文件: ${OpenCV_INCLUDE_DIRS}")

add_executable(demo1 demo1.cpp)

target_include_directories(demo1 PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(demo1 PRIVATE ${OpenCV_LIBS})
