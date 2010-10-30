
spec2def(ufat.dll ufat.spec)

add_library(ufat SHARED
    ufat.c
    ufat.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ufat.def)

set_module_type(ufat win32dll)

target_link_libraries(ufat vfatlib)

add_importlibs(ufat kernel32 ntdll)

add_cab_target(ufat 1)
