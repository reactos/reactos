
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

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
