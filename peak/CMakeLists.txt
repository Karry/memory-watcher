
set(HEADER_FILES
    Peak.h
    )

set(SOURCE_FILES
    Peak.cpp)

add_executable(memory-peak ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET memory-peak PROPERTY CXX_STANDARD 11)

target_include_directories(memory-peak PRIVATE
    ${WATCHER_UTILS_INCLUDE_DIR}
    )

target_link_libraries(memory-peak
    Qt5::Core
    Qt5::Sql
    memory-watcher-utils
    )

install(TARGETS memory-peak
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

