
spec2def(untfs.dll untfs.spec)

add_library(untfs SHARED
    untfs.c
    untfs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/untfs.def)

set_module_type(untfs win32dll)

target_link_libraries(untfs ntfslib)

add_importlibs(untfs kernel32 ntdll)

add_cab_target(untfs 1)
