cmake_minimum_required(VERSION 3.10)
project(main)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)


add_executable(main main.cc)

target_include_directories(main PRIVATE ${OpenCV_INCLUDE_DIRS}
                                        ${CMAKE_CURRENT_SOURCE_DIR}/ThirdLib/Galaxy/inc)
target_link_libraries(main PRIVATE ${OpenCV_LIBS} ${CMAKE_CURRENT_SOURCE_DIR}/ThirdLib/Galaxy/lib)
