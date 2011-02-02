
add_library(normaliz SHARED dummy.c ${CMAKE_CURRENT_SOURCE_DIR}/normaliz.def)

set_entrypoint(normaliz 0)

add_importlibs(normaliz kernel32)

add_dependencies(normaliz psdk buildno_header)
