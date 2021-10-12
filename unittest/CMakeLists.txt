#------------------------------------------------------------------------------
# Unit tests via Catch framework
#
# For testing on the function/class level.


set(SRCTEST
    testmain.cpp

    ../utils/ProcessId.cpp ../utils/ProcessId.h
)

add_executable(unittests EXCLUDE_FROM_ALL ${SRCTEST})
target_compile_definitions(unittests PRIVATE UNIT_TESTS) #add -DUNIT_TESTS define

target_link_libraries (unittests
    ${CMAKE_THREAD_LIBS_INIT} #threading
    Catch2::Catch2
    Qt5::Core
    Qt5::Sql)


add_test(NAME unittests
    COMMAND $<TARGET_FILE:unittests> -r junit -o unittests.junit.xml
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
