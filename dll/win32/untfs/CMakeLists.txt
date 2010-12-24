
spec2def(untfs.dll untfs.spec)

list(APPEND SOURCE
    untfs.c
    untfs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/untfs.def)

add_library(untfs SHARED ${SOURCE})

set_module_type(untfs win32dll)
target_link_libraries(untfs ntfslib)
add_importlibs(untfs msvcrt kernel32 ntdll)
add_cab_target(untfs 1)
