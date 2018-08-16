
set(HEADER_FILES
    Utils.h
    ThreadPool.h
    Storage.h
    QVariantConverters.h)

set(SOURCE_FILES
    Utils.cpp
    ThreadPool.cpp
    Storage.cpp)

add_library(memory-watcher-utils ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET memory-watcher-utils PROPERTY CXX_STANDARD 11)

target_include_directories(memory-watcher-utils PRIVATE
    )

target_link_libraries(memory-watcher-utils
    Qt5::Core
    Qt5::Sql
    )

set(WATCHER_UTILS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
