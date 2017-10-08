
spec2def(timedate.cpl timedate.spec)

list(APPEND SOURCE
    clock.c
    dateandtime.c
    internettime.c
    monthcal.c
    ntpclient.c
    timedate.c
    timezone.c
    timedate.h)

file(GLOB timedate_rc_deps resources/*.*)
add_rc_deps(timedate.rc ${timedate_rc_deps})

add_library(timedate SHARED
    ${SOURCE}
    timedate.rc
    ${CMAKE_CURRENT_BINARY_DIR}/timedate.def)

set_module_type(timedate cpl UNICODE)
add_importlibs(timedate advapi32 user32 gdi32 comctl32 ws2_32 iphlpapi msvcrt kernel32)
add_pch(timedate timedate.h SOURCE)
add_cd_file(TARGET timedate DESTINATION reactos/system32 FOR all)
