
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__ -D__ROS_LONG64__)
spec2def(uxtheme.dll uxtheme.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    buffer.c
    draw.c
    main.c
    metric.c
    msstyles.c
    ncscrollbar.c
    nonclient.c
    property.c
    stylemap.c
    system.c
    themehooks.c
    uxini.c
    uxthemep.h
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme_stubs.c)

add_library(uxtheme MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme.def)

set_module_type(uxtheme win32dll)
target_link_libraries(uxtheme wine)
add_delay_importlibs(uxtheme msimg32)
add_importlibs(uxtheme user32 advapi32 gdi32 msvcrt kernel32 ntdll)
add_pch(uxtheme uxthemep.h SOURCE)
add_cd_file(TARGET uxtheme DESTINATION reactos/system32 FOR all)
