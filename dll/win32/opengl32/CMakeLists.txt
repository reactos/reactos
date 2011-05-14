
set_unicode()

add_definitions(-D_DLL -D__USE_CRTIMP)

spec2def(opengl32.dll opengl32.spec)

list(APPEND SOURCE
    font.c
    gl.c
    opengl32.c
    wgl.c
    ${CMAKE_CURRENT_BINARY_DIR}/opengl32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/opengl32.def)

add_library(opengl32 SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/opengl32_opengl32.h.gch
    ${SOURCE})

set_module_type(opengl32 win32dll)

add_importlibs(opengl32
    msvcrt
    gdi32
    user32
    advapi32
    glu32
    kernel32
    ntdll)

add_pch(opengl32 ${CMAKE_CURRENT_SOURCE_DIR}/opengl32.h ${SOURCE})

add_cd_file(TARGET opengl32 DESTINATION reactos/system32 FOR all)
add_importlib_target(opengl32.spec)
