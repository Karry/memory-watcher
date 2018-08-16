
set(HEADER_FILES
    SmapsWatcher.h
    Watcher.h
    Feeder.h)

set(SOURCE_FILES
    SmapsWatcher.cpp
    Watcher.cpp
    Feeder.cpp)

add_executable(memory-watcher ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET memory-watcher PROPERTY CXX_STANDARD 11)

target_include_directories(memory-watcher PRIVATE
    ${WATCHER_UTILS_INCLUDE_DIR}
    )

target_link_libraries(memory-watcher
    Qt5::Core
    Qt5::Sql
    memory-watcher-utils
    )


