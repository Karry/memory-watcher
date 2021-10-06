
set(HEADER_FILES
    Replay.h
    )

set(SOURCE_FILES
    Replay.cpp)

add_executable(memory-replay ${SOURCE_FILES} ${HEADER_FILES})

set_property(TARGET memory-replay PROPERTY INTERPROCEDURAL_OPTIMIZATION ${MEMORY_WATCHER_ENABLE_IPO})

target_include_directories(memory-replay PRIVATE
    ${WATCHER_UTILS_INCLUDE_DIR}
    )

target_link_libraries(memory-replay
    Qt5::Core
    Qt5::Sql
    memory-watcher-utils
    )

install(TARGETS memory-replay
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
