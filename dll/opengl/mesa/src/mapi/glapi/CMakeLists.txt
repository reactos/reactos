
add_definitions(
    -DMAPI_MODE_UTIL
    -D_GDI32_ # prevent gl* being declared __declspec(dllimport) in MS headers
    -DBUILD_GL32 # declare gl* as __declspec(dllexport) in Mesa headers
    -D_GLAPI_NO_EXPORTS # prevent _glapi_* from being declared __declspec(dllimport)
    -DKHRONOS_DLL_EXPORTS
)

if((ARCH STREQUAL "i386") AND (NOT MSVC))
    list(APPEND SOURCE glapi_x86.S)
    add_definitions(
        -DUSE_X86_ASM
        -DSTDCALL_API)
endif()

include_directories(
    ../../mesa
    ..)

list(APPEND SOURCE
    glapi_dispatch.c
    glapi_entrypoint.c
    glapi_getproc.c
    glapi_nop.c
    glthread.c
    glapi.c
    ../mapi/u_current.c
    ../mapi/u_execmem.c
    ../mapi/u_thread.c)

if(NOT MSVC)    
    add_compile_flags("-Wno-unused-function")
endif()

add_library(mesa_glapi STATIC EXCLUDE_FROM_ALL ${SOURCE})