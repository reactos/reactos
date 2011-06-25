
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)
set_rc_compiler()

spec2def(uxtheme.dll uxtheme.spec ADD_IMPORTLIB GENERATE_STUBS)

list(APPEND SOURCE
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
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme.def)

add_library(uxtheme SHARED ${SOURCE})

set_module_type(uxtheme win32dll)

target_link_libraries(uxtheme wine)

add_importlibs(uxtheme user32 advapi32 gdi32 msimg32 msvcrt kernel32 ntdll)

add_cd_file(TARGET uxtheme DESTINATION reactos/system32 FOR all)
add_importlib_target(uxtheme.spec)
