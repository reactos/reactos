

spec2def(opengl32.dll opengl32.spec ADD_IMPORTLIB)

add_definitions(
    -D_GDI32_ # prevent gl* being declared __declspec(dllimport) in MS headers
    -DBUILD_GL32 # declare gl* as __declspec(dllexport) in Mesa headers
)

list(APPEND SOURCE
    font.c
    gl.c
    opengl32.c
    wgl.c
    ${CMAKE_CURRENT_BINARY_DIR}/opengl32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/opengl32.def)

add_library(opengl32 SHARED ${SOURCE})

set_module_type(opengl32 win32dll UNICODE)

add_importlibs(opengl32
    msvcrt
    gdi32
    user32
    advapi32
    glu32
    kernel32
    ntdll)

add_pch(opengl32 opengl32.h)

add_cd_file(TARGET opengl32 DESTINATION reactos/system32 FOR all)

