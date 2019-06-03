
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    assisted.c
    internal.c
    line.c
    phone.c
    precomp.h)

add_library(tapi32 MODULE ${SOURCE})
spec2def(tapi32 tapi32.dll tapi32.spec)

set_module_type(tapi32 win32dll)
target_link_libraries(tapi32 wine)
add_importlibs(tapi32 advapi32 msvcrt kernel32 ntdll)
add_pch(tapi32 precomp.h SOURCE)
add_cd_file(TARGET tapi32 DESTINATION reactos/system32 FOR all)
