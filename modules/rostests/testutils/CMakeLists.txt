
add_library(testutils STATIC
    XStateHelpers.cpp
)

add_dependencies(testutils xdk)

# add inc as public include
target_include_directories(testutils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
