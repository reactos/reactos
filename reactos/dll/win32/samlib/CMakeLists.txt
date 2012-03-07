
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

set_rc_compiler()
spec2def(samlib.dll samlib.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    dllmain.c
    samlib.c
    samlib.rc
    ${CMAKE_CURRENT_BINARY_DIR}/samlib_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/samlib.def)

add_library(samlib SHARED ${SOURCE})
set_module_type(samlib win32dll)
add_importlibs(samlib advapi32 msvcrt kernel32 ntdll)
add_pch(samlib precomp.h)
add_cd_file(TARGET samlib DESTINATION reactos/system32 FOR all)

