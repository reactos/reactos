
add_definitions(
    -D_GDI32_ # prevent gl* being declared __declspec(dllimport) in MS headers
    -DBUILD_GL32 # declare gl* as __declspec(dllexport) in Mesa headers
    -D_GLAPI_NO_EXPORTS # prevent _glapi_* from being declared __declspec(dllimport)
    -DWIN32_LEAN_AND_MEAN
)
if(NOT MSVC)
    add_compile_flags("-Wno-implicit-function-declaration")
endif()

include_directories(
    ../../../mapi
    ../../../mesa
    .)

list( APPEND SOURCE
    stw_context.c
    stw_device.c
    stw_ext_context.c
    stw_ext_extensionsstring.c
    stw_ext_gallium.c
    stw_ext_pbuffer.c
    stw_ext_pixelformat.c
    stw_ext_swapinterval.c
    stw_framebuffer.c
    stw_getprocaddress.c
    stw_pixelformat.c
    stw_st.c
    stw_tls.c
    stw_wgl.c
)

add_library(gallium_wgl STATIC EXCLUDE_FROM_ALL ${SOURCE})