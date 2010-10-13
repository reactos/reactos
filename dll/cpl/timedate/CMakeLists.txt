
set_unicode()

spec2def(timedate ${CMAKE_CURRENT_SOURCE_DIR}/timedate.spec ${CMAKE_CURRENT_BINARY_DIR}/timedate.def)

add_library(timedate SHARED
    clock.c
    dateandtime.c
    internettime.c
    monthcal.c
    ntpclient.c
    timedate.c
    timezone.c
    timedate.rc)

set_module_type(timedate cpl)

target_link_libraries(timedate ${CMAKE_CURRENT_BINARY_DIR}/timedate.def)

add_importlibs(timedate
    advapi32
    user32
    gdi32
    comctl32
    ws2_32
    iphlpapi)

add_dependencies(timedate timedate_def)
