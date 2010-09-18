


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

set_target_properties(uxtheme PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(uxtheme
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme.def
    wine
    -luser32
    -ladvapi32
    -lgdi32
    -lmsimg32
    mingw_common)

add_dependencies(uxtheme uxtheme_def psdk)
