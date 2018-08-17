cmake_minimum_required(VERSION 3.0)

project(memory-watcher C CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON) # build should fail when compiler don't support standard defined by CMAKE_CXX_STANDARD

add_definitions( -Wall -pedantic -fPIC -Wno-unused-function)
add_definitions(-std=c++11)

find_package( Qt5 ${QT_MIN_VERSION} 
              REQUIRED NO_MODULE COMPONENTS 
              Core
#              Gui
#              Xml
#              Svg
#              Qml
#              Quick
              Sql
)

add_definitions(-fPIC)
add_definitions(${QT_DEFINITIONS})
add_definitions(${Qt5Core_DEFINITIONS})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(utils)
add_subdirectory(watcher)
add_subdirectory(peak)
