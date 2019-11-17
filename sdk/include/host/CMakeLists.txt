
add_library(host_includes INTERFACE)
target_include_directories(host_includes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
