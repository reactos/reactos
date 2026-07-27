
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

include_directories(${VKD3D_INCLUDE_DIRS})

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
    viewport.c)

add_library(ddraw MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ddraw_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/ddraw.def)

add_idl_reg_scripts(ddraw registry ddraw_classes.idl)

if(MSVC)
    # C5286: implicit conversion between enum types
    target_compile_options(ddraw PRIVATE /wd5286)
endif()

set_module_type(ddraw win32dll UNICODE)
target_link_libraries(ddraw wine wine_dll_register wine_dll_canunload dxguid uuid ${PSEH_LIB})
add_importlibs(ddraw d3dwine user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(ddraw wineheaders d3d_idl_headers)
add_cd_file(TARGET ddraw DESTINATION reactos/system32 FOR all)
set_wine_module(ddraw)
