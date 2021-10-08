
# project version
set(MEMORY_WATCHER_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Version.h.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/Version.h")

set(HEADER_FILES
    CmdLineParsing.h
    QVariantConverters.h
    SmapsRange.h
    StatM.h
    Storage.h
    String.h
    ThreadPool.h
    Utils.h)

set(SOURCE_FILES
    CmdLineParsing.cpp
    SmapsRange.cpp
    Storage.cpp
    String.cpp
    ThreadPool.cpp
    Utils.cpp)

add_library(memory-watcher-utils ${SOURCE_FILES} ${HEADER_FILES})

set_property(TARGET memory-watcher-utils PROPERTY INTERPROCEDURAL_OPTIMIZATION ${MEMORY_WATCHER_ENABLE_IPO})

target_include_directories(memory-watcher-utils PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(memory-watcher-utils
    Qt5::Core
    Qt5::Sql
    )

set(WATCHER_UTILS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
