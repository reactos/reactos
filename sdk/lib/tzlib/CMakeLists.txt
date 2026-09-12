
add_library(tzlib STATIC tzlib.c)
add_dependencies(tzlib xdk)
target_include_directories(tzlib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
