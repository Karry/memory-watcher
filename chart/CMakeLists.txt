
set(HEADER_FILES
    Chart.h)

set(SOURCE_FILES
    Chart.cpp)

add_executable(memory-chart ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(memory-chart PRIVATE
    ${WATCHER_UTILS_INCLUDE_DIR}
    )

target_link_libraries(memory-chart
    Qt5::Core
    Qt5::Sql
    Qt5::Gui
    Qt5::Charts
    memory-watcher-utils
    )

install(TARGETS memory-chart
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

