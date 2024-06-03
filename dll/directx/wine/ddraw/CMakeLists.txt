
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -DUSE_WIN32_OPENGL)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

spec2def(ddraw.dll ddraw.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    clipper.c
    ddraw.c
    device.c
    executebuffer.c
    light.c
    main.c
    material.c
    palette.c
    surface.c
    utils.c
    vertexbuffer.c
    viewport.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/ddraw_stubs.c)

if(MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #target_compile_options(ddraw PRIVATE "/FIwine/typeof.h")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_OPTIONS "/FIwine/typeof.h")
endif()

add_library(ddraw MODULE
    ${SOURCE}
    ddraw.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ddraw.def)

set_module_type(ddraw win32dll)
target_link_libraries(ddraw wine uuid dxguid ${PSEH_LIB})
add_importlibs(ddraw advapi32 gdi32 user32 d3dwine msvcrt kernel32 ntdll)
add_dependencies(ddraw wineheaders)
add_pch(ddraw precomp.h SOURCE)
add_cd_file(TARGET ddraw DESTINATION reactos/system32 FOR all)
