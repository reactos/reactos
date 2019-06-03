
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

list(APPEND SOURCE
    misc/dllmain.c
    misc/event.c
    misc/helpers.c
    misc/sndrcv.c
    misc/stubs.c
    msafd.h)

add_library(msafd MODULE
    ${SOURCE}
    msafd.rc)
spec2def(msafd msafd.dll msafd.spec)

set_module_type(msafd win32dll UNICODE)
target_link_libraries(msafd wine)
add_importlibs(msafd advapi32 ws2_32 msvcrt kernel32 ntdll)
add_pch(msafd msafd.h SOURCE)
add_cd_file(TARGET msafd DESTINATION reactos/system32 FOR all)
