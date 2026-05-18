file(GLOB SOURCES
    "*.cpp"
    "*.h"
)

add_executable(${COMPONENT}.test ${SOURCES})

target_include_directories(${COMPONENT}.test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${COMPONENT}.test PUBLIC
    GTest::gtest_main
    ${COMPONENT}
)