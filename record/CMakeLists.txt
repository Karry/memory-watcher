
set(HEADER_FILES
    MemoryWatcher.h
    Record.h
    Feeder.h)

set(SOURCE_FILES
    MemoryWatcher.cpp
    Record.cpp
    Feeder.cpp)

add_executable(memory-record ${SOURCE_FILES} ${HEADER_FILES})

set_property(TARGET memory-record PROPERTY INTERPROCEDURAL_OPTIMIZATION ${MEMORY_WATCHER_ENABLE_IPO})

target_include_directories(memory-record PRIVATE
    ${WATCHER_UTILS_INCLUDE_DIR}
    )

target_link_libraries(memory-record
    Qt5::Core
    Qt5::Sql
    memory-watcher-utils
    )

install(TARGETS memory-record
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

