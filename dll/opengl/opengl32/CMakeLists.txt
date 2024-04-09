
spec2def(opengl32.dll opengl32.spec ADD_IMPORTLIB)

include_directories(../mesa)

add_definitions(
    -D_GDI32_ # prevent gl* being declared __declspec(dllimport) in MS headers
    -DBUILD_GL32 # declare gl* as __declspec(dllexport) in Mesa headers
    -D_GLAPI_NO_EXPORTS # prevent _glapi_* from being declared __declspec(dllimport)
)

if(OPENGL32_USE_TLS)
    add_definitions(-DOPENGL32_USE_TLS)
endif()

list(APPEND SOURCE
    apistubs.c
    dllmain.c
    icdload.c
    swimpl.c
    wgl.c
    wgl_font.c
    opengl32.h
    ${CMAKE_CURRENT_BINARY_DIR}/opengl32_stubs.c)

set_source_files_properties(gcrt0.o libgmon.a PROPERTIES EXTERNAL_OBJECT TRUE)

if(ARCH STREQUAL "i386")
    # Optimisation: use asm trampolines to ICD provided functions
    add_asm_files(opengl32_asm
        glapi_x86.s
    )
endif()

add_library(opengl32 MODULE
    ${SOURCE}
    ${opengl32_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/opengl32.def)

target_link_libraries(opengl32
    wine
    ${PSEH_LIB}
    mesa_drv_common
    mesa_swrast
    mesa_swrast_setup
    mesa_tnl
    mesa_main
    mesa_vbo
    mesa_math
)

if((ARCH STREQUAL "i386") AND (NOT MSVC))
    target_link_libraries(opengl32 mesa_x86)
endif()

set_module_type(opengl32 win32dll)
add_importlibs(opengl32 gdi32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(opengl32 opengl32.h SOURCE)
add_cd_file(TARGET opengl32 DESTINATION reactos/system32 FOR all)
