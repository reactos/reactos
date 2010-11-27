
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(uxtheme.dll uxtheme.spec)

add_library(uxtheme SHARED
    buffer.c
    draw.c
    main.c
    metric.c
    msstyles.c
    property.c
    stylemap.c
    system.c
    uxini.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme.def)

set_module_type(uxtheme win32dll)

target_link_libraries(uxtheme wine)

add_importlibs(uxtheme user32 advapi32 gdi32 msimg32 msvcrt kernel32 ntdll)

add_cab_target(uxtheme 1)
add_importlib_target(uxtheme.spec)
