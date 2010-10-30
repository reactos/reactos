
spec2def(uext2 ${CMAKE_CURRENT_SOURCE_DIR}/uext2.spec ${CMAKE_CURRENT_BINARY_DIR}/uext2.def)

add_library(uext2 SHARED uext2.c uext2.rc)

set_module_type(uext2 win32dll)

target_link_libraries(uext2
    ${CMAKE_CURRENT_BINARY_DIR}/uext2.def
    ext2lib)

add_importlibs(uext2 kernel32 ntdll)
add_dependencies(uext2 uext2_def)
add_cab_target(uext2 1)
