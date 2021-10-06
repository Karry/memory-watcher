
set(HEADER_FILES
    MemoryLoader.h
    Load.h
    Feeder.h)

set(SOURCE_FILES
    MemoryLoader.cpp
    Load.cpp
    Feeder.cpp)

add_executable(memory-load-smaps ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(memory-load-smaps PRIVATE
    ${WATCHER_UTILS_INCLUDE_DIR}
    )

target_link_libraries(memory-load-smaps
    Qt5::Core
    Qt5::Sql
    memory-watcher-utils
    )

install(TARGETS memory-load-smaps
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
