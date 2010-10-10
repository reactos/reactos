
add_library(normaliz SHARED dummy.c)

set_entrypoint(normaliz 0)

target_link_libraries(normaliz ${CMAKE_CURRENT_SOURCE_DIR}/normaliz.def)

add_dependencies(normaliz psdk buildno_header)
