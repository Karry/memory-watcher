
set(HEADER_FILES
    Chart.h)

set(SOURCE_FILES
    Chart.cpp)

add_executable(memory-chart ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET memory-chart PROPERTY CXX_STANDARD 11)

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


