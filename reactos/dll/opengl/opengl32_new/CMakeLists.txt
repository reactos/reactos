
spec2def(opengl32.dll opengl32.spec ADD_IMPORTLIB)

add_definitions(
    -D_GDI32_ # prevent gl* being declared __declspec(dllimport) in MS headers
    -DBUILD_GL32 # declare gl* as __declspec(dllexport) in Mesa headers
)

# useful to test under windows <> w2k3
set(OPENGL32_USE_TLS TRUE)

if(OPENGL32_USE_TLS)
    add_definitions(-DOPENGL32_USE_TLS)
endif()

list(APPEND SOURCE
    apistubs.c
    dllmain.c
    icdload.c
    swimpl.c
    wgl.c
    ${CMAKE_CURRENT_BINARY_DIR}/opengl32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/opengl32.def
)

if((ARCH STREQUAL "i386") AND (NOT OPENGL32_USE_TLS))
    # Optimisation: use asm trampolines to ICD provided functions
    list(APPEND SOURCE
        glapi_x86.s
    )
endif()

add_library(opengl32_new SHARED ${SOURCE})
target_link_libraries(opengl32_new wine ${PSEH_LIB})
set_module_type(opengl32_new win32dll)
add_importlibs(opengl32_new gdi32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET opengl32_new DESTINATION reactos/system32 FOR all)