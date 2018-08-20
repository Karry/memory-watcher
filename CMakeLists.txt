cmake_minimum_required(VERSION 3.0)

project(memory-watcher C CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON) # build should fail when compiler don't support standard defined by CMAKE_CXX_STANDARD

add_definitions( -Wall -pedantic -fPIC -Wno-unused-function)
add_definitions(-std=c++11)

set(QT_MIN_VERSION "5.2.0")
find_package( Qt5 ${QT_MIN_VERSION} 
              REQUIRED NO_MODULE COMPONENTS 
              Core
              Sql
              )

find_package(Qt5Gui CONFIG QUIET)
find_package(Qt5Charts CONFIG QUIET)

add_definitions(-fPIC)
add_definitions(${QT_DEFINITIONS})
add_definitions(${Qt5Core_DEFINITIONS})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(utils)
add_subdirectory(record)
add_subdirectory(peak)
add_subdirectory(replay)

if (Qt5Gui_FOUND AND Qt5Charts_FOUND)
  message(STATUS "Build optional chart tool")
  add_subdirectory(chart)
else()
  message(WARNING "Optional chart tool will not be compiled (require Qt5Gui and Qt5Charts)")
endif()
