cmake_minimum_required(VERSION 3.10)

project(memory-watcher VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON) # build should fail when compiler don't support standard defined by CMAKE_CXX_STANDARD
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE OFF)

add_definitions( -Wall -Wextra -pedantic -Wno-unused-function)
add_definitions( -fno-omit-frame-pointer -fstack-protector-all -fPIC )

# Do *NOT* turn assertions off in release and rel-with-deb-info modes.
# CMakes does that by default despite any recent documentation
# recommending against the practice.
string( REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
string( REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
string( REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
string( REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)
if (NOT BUILD_TYPE_LOWER STREQUAL "debug")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT result OUTPUT output)
  if(result)
    set(MEMORY_WATCHER_ENABLE_IPO TRUE)
  else()
    message(WARNING "IPO is not supported")
    set(MEMORY_WATCHER_ENABLE_IPO FALSE)
  endif()
else()
  set(MEMORY_WATCHER_ENABLE_IPO FALSE)
endif()

option(SANITIZER "Build with sanitizer" none)
if(SANITIZER STREQUAL "address")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
elseif(SANITIZER STREQUAL "undefined")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
elseif(SANITIZER STREQUAL "thread")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
else()
  set(SANITIZER "none")
endif()

set(QT_MIN_VERSION "5.2.0")
find_package( Qt5 ${QT_MIN_VERSION} 
              REQUIRED NO_MODULE COMPONENTS 
              Core
              Sql
              )

find_package(Qt5Gui CONFIG QUIET)
find_package(Qt5Charts CONFIG QUIET)

add_definitions(${QT_DEFINITIONS})
add_definitions(${Qt5Core_DEFINITIONS})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(utils)

option(MEMORY_WATCHER_BUILD_RECORD "Enable build of record tool" ON)
if(MEMORY_WATCHER_BUILD_RECORD)
  add_subdirectory(record)
endif()

option(MEMORY_WATCHER_BUILD_LOAD "Enable build of load tool" ON)
if(MEMORY_WATCHER_BUILD_LOAD)
  add_subdirectory(load)
endif()

option(MEMORY_WATCHER_BUILD_PEAK "Enable build of peak tool" ON)
if(MEMORY_WATCHER_BUILD_PEAK)
  add_subdirectory(peak)
endif()

option(MEMORY_WATCHER_BUILD_REPLAY "Enable build of reploay tool" ON)
if(MEMORY_WATCHER_BUILD_REPLAY)
  add_subdirectory(replay)
endif()

if (Qt5Gui_FOUND AND Qt5Charts_FOUND)
  set(MEMORY_WATCHER_BUILD_CHART_CACHE ON)
else()
  set(MEMORY_WATCHER_BUILD_CHART_CACHE OFF)
endif()
option(MEMORY_WATCHER_BUILD_CHART "Enable build of chart tool" ${MEMORY_WATCHER_BUILD_CHART_CACHE})
if(MEMORY_WATCHER_BUILD_CHART)
  if (NOT Qt5Gui_FOUND OR NOT Qt5Charts_FOUND)
    message(SEND_ERROR "Qt5Gui and Qt5Charts is required for chart tool")
    set(MEMORY_WATCHER_BUILD_CHART OFF)
  else()
    message(STATUS "Build optional chart tool")
    add_subdirectory(chart)
  endif()
else()
  message(STATUS "Optional chart tool will not be compiled (require Qt5Gui and Qt5Charts)")
endif()

message(STATUS "memory watcher ${PROJECT_VERSION} build configuration:")

message(STATUS "")
message(STATUS "Build options:")
message(STATUS " Build type                      ${CMAKE_BUILD_TYPE}")
message(STATUS " Interprocedural optimizations   ${MEMORY_WATCHER_ENABLE_IPO}")
message(STATUS " Sanitizer                       ${SANITIZER}")

message(STATUS "")
message(STATUS "Requiered dependencies:")
message(STATUS "Qt libraries (version >= ${QT_MIN_VERSION}):")
message(STATUS " Core:                           ${Qt5Core_FOUND}")
message(STATUS " Sql:                            ${Qt5Sql_FOUND}")
message(STATUS " Gui:                            ${Qt5Gui_FOUND}")
message(STATUS " Charts:                         ${Qt5Charts_FOUND}")

message(STATUS "")
message(STATUS "Build tools:")
message(STATUS " memory-record:                  ${MEMORY_WATCHER_BUILD_RECORD}")
message(STATUS " memory-load-smaps:              ${MEMORY_WATCHER_BUILD_LOAD}")
message(STATUS " memory-peak:                    ${MEMORY_WATCHER_BUILD_PEAK}")
message(STATUS " memory-replay:                  ${MEMORY_WATCHER_BUILD_REPLAY}")
message(STATUS " memory-chart:                   ${MEMORY_WATCHER_BUILD_CHART}")
if(CCACHE_PROGRAM)
  message(STATUS "Using ccache:                    ${CCACHE_PROGRAM}")
endif()