
include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers)

spec2def(msafd.dll msafd.spec)

list(APPEND SOURCE
    misc/dllmain.c
    misc/event.c
    misc/helpers.c
    misc/sndrcv.c
    misc/stubs.c
    msafd.h)

add_library(msafd SHARED
    ${SOURCE}
    msafd.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msafd.def)

set_module_type(msafd win32dll UNICODE)
target_link_libraries(msafd wine)
add_importlibs(msafd advapi32 msvcrt kernel32 ntdll)
add_pch(msafd msafd.h SOURCE)
add_cd_file(TARGET msafd DESTINATION reactos/system32 FOR all)
