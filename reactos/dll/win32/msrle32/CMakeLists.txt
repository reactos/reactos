
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(msrle32.dll msrle32.spec)

list(APPEND SOURCE
    msrle32.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msrle32.def)

add_library(msrle32 SHARED ${SOURCE})
set_module_type(msrle32 win32dll)
target_link_libraries(msrle32 wine)
add_importlibs(msrle32 msvcrt winmm user32 kernel32 ntdll)
add_cd_file(TARGET msrle32 DESTINATION reactos/system32 FOR all)
