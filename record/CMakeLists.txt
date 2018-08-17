
set(HEADER_FILES
    SmapsWatcher.h
    Record.h
    Feeder.h)

set(SOURCE_FILES
    SmapsWatcher.cpp
    Record.cpp
    Feeder.cpp)

add_executable(memory-record ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET memory-record PROPERTY CXX_STANDARD 11)

target_include_directories(memory-record PRIVATE
    ${WATCHER_UTILS_INCLUDE_DIR}
    )

target_link_libraries(memory-record
    Qt5::Core
    Qt5::Sql
    memory-watcher-utils
    )


