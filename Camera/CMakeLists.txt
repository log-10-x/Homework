cmake_minimum_required(VERSION 3.10)
project(Demo2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

set(GALAXY_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_lib/Galaxy_camera")
find_library(GALAXY_LIBRARY
	NAMES gxiapi
	PATHS "${GALAXY_SDK_DIR}/lib/x86_64"
	NO_DEFAULT_PATH
)

if(NOT GALAXY_LIBRARY)
	message(FATAL_ERROR "Galaxy SDK library libgxiapi.so not found in ${GALAXY_SDK_DIR}/lib/x86_64")
endif()

add_executable(demo2 demo2.cpp)

target_include_directories(demo2 PRIVATE
	"${GALAXY_SDK_DIR}/inc"
	${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(demo2 PRIVATE
	${OpenCV_LIBS}
	"${GALAXY_LIBRARY}"
)

set_target_properties(demo2 PROPERTIES
	BUILD_RPATH "${GALAXY_SDK_DIR}/lib/x86_64"
)

