
spec2def(timedate.cpl timedate.spec)

add_library(timedate SHARED
    clock.c
    dateandtime.c
    internettime.c
    monthcal.c
    ntpclient.c
    timedate.c
    timezone.c
    timedate.rc
    ${CMAKE_CURRENT_BINARY_DIR}/timedate.def)

set_module_type(timedate cpl UNICODE)

add_importlibs(timedate
    advapi32
    user32
    gdi32
    comctl32
    ws2_32
    iphlpapi
    msvcrt
    kernel32)

add_pch(timedate timedate.h)
add_cd_file(TARGET timedate DESTINATION reactos/system32 FOR all)
