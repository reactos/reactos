
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(objsel.dll objsel.spec)

list(APPEND SOURCE
    factory.c
    objsel.c
    precomp.h)

add_library(objsel MODULE
    ${SOURCE}
    objsel.rc
    ${CMAKE_CURRENT_BINARY_DIR}/objsel.def)

set_module_type(objsel win32dll)
target_link_libraries(objsel uuid wine)
add_importlibs(objsel ole32 advapi32 msvcrt kernel32 ntdll)
add_pch(objsel precomp.h SOURCE)
add_cd_file(TARGET objsel DESTINATION reactos/system32 FOR all)
