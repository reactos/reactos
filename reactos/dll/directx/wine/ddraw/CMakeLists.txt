
add_definitions(
    -D__WINESRC__
    -DUSE_WIN32_OPENGL)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

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
    stubs.c
    surface.c
    utils.c
    vertexbuffer.c
    viewport.c
    ${CMAKE_CURRENT_BINARY_DIR}/ddraw_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/ddraw.def)

add_library(ddraw SHARED
    ${SOURCE}
    ddraw.rc)

if(MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #add_target_compile_flags(ddraw "/FIwine/typeof.h")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "/FIwine/typeof.h")
endif()

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
