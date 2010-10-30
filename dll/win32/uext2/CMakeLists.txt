
spec2def(uext2.dll uext2.spec)

add_library(uext2 SHARED
    uext2.c
    uext2.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uext2.def)

set_module_type(uext2 win32dll)

target_link_libraries(uext2 ext2lib)

add_importlibs(uext2 kernel32 ntdll)

add_cab_target(uext2 1)
