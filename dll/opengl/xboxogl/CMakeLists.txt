
spec2def(xboxogl.dll xboxogl.spec ADD_IMPORTLIB)

include_directories(../opengl32)

# OpenGL function declarations in the system gl.h are __stdcall (GLAPIENTRY).
# Mesa's headers in dll/opengl/mesa are not visible here — we use the SDK ones.
add_definitions(
    -D_GDI32_  # prevent gl* being declared __declspec(dllimport)
)

list(APPEND SOURCE
    dllmain.c
    icd.c
    dispatch.c
    real.c
    xboxogl.h)

add_library(xboxogl MODULE
    ${SOURCE}
    xboxogl.rc
    ${CMAKE_CURRENT_BINARY_DIR}/xboxogl.def)

set_module_type(xboxogl win32dll)
add_importlibs(xboxogl gdi32 user32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(xboxogl psdk)

if(SARCH STREQUAL "xbox")
    add_cd_file(TARGET xboxogl DESTINATION reactos/system32 FOR all)
endif()
