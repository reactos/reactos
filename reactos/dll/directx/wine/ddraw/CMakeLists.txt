
add_definitions(
    -D__WINESRC__
    -DUSE_WIN32_OPENGL)

if(MSVC)
    add_definitions(/FIwine/typeof.h)
endif()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(ddraw.dll ddraw.spec)

list(APPEND SOURCE
    clipper.c
    ddraw.c
    ddraw.rc
    device.c
    executebuffer.c
    light.c
    main.c
    material.c
    palette.c
    stubs.c
    surface.c
    utils.c
    vertexbuffer.c
    viewport.c
    ${CMAKE_CURRENT_BINARY_DIR}/ddraw_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/ddraw.def)

add_library(ddraw SHARED ${SOURCE})
set_module_type(ddraw win32dll)

target_link_libraries(ddraw
    wine
    uuid
    dxguid
    ${PSEH_LIB})

add_importlibs(ddraw
    advapi32
    gdi32
    ole32
    user32
    wined3d
    msvcrt
    kernel32
    ntdll)

add_dependencies(ddraw wineheaders)
add_pch(ddraw ddraw_private.h)
add_cd_file(TARGET ddraw DESTINATION reactos/system32 FOR all)
add_importlib_target(ddraw.spec ddraw.dll)
