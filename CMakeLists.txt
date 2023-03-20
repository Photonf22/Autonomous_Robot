cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0074 NEW)    # respect <PACKAGE>_ROOT variables in "find_package"
include(CMakePrintHelpers)
set(CMAKE_CXX_STANDARD 14)
add_definitions(-std=c++11)

set(CXX_FLAGS "-Wall" "-pedantic")
set(CMAKE_CXX_FLAGS, "${CXX_FLAGS}")

project(Autonomous_Robot)

find_package(OpenCV 4.1 REQUIRED)
find_package(pylon 7.2.1 REQUIRED)
find_package(Iconv REQUIRED)
find_package(Eigen3 3.4.0 REQUIRED NO_MODULE)
find_package(Qt5Core REQUIRED)
find_package(Qt5Test REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Concurrent)
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS})
add_definitions(${OpenCV_DEFINITIONS})
find_package(Ceres REQUIRED)

include_directories(
        #add other includes directories 
        ${Pylon_INCLUDE_DIRS}
)
include_directories(/opt/pylon/include)
add_executable(Autonomous_Robot Grab.cpp)
target_link_libraries (Autonomous_Robot PRIVATE ${OpenCV_LIBS})
target_link_libraries( Autonomous_Robot PRIVATE pylon::pylon )
install( TARGETS Autonomous_Robot )
