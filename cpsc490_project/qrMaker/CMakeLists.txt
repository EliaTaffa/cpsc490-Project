cmake_minimum_required(VERSION 2.8)
project( qrMaker )
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories( include )

file(GLOB SOURCES "src/*.cpp")
add_executable( qrMaker ${SOURCES} )
target_link_libraries( qrMaker ${OpenCV_LIBS} )