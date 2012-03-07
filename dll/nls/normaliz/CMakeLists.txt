
spec2def(normaliz.dll normaliz.spec)

add_library(normaliz SHARED dummy.c ${CMAKE_CURRENT_BINARY_DIR}/normaliz.def)

set_entrypoint(normaliz 0)

add_importlibs(normaliz kernel32)

add_dependencies(normaliz psdk)
