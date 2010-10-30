
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(uxtheme ${CMAKE_CURRENT_SOURCE_DIR}/uxtheme.spec ${CMAKE_CURRENT_BINARY_DIR}/uxtheme.def)

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
    version.rc)

set_module_type(uxtheme win32dll)

target_link_libraries(uxtheme
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme.def
    wine)

add_importlibs(uxtheme user32 advapi32 gdi32 msimg32 kernel32 ntdll)
add_dependencies(uxtheme uxtheme_def)
add_cab_target(uxtheme 1)
