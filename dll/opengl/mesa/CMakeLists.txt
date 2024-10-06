
include_directories(.)

# our DBG definitions conflict with mesa source code
remove_definitions(-DDBG=1 -DDBG=0)

add_definitions(
    -DWIN32
    -D_WINDOWS
    -D_DLL
    -DFEATURE_GL=1
    -D_GDI32_           # prevent gl* being declared __declspec(dllimport) in MS headers
    -DBUILD_GL32        # declare gl* as __declspec(dllexport) in Mesa headers
    -D_GLAPI_NO_EXPORTS # prevent _glapi_* from being declared __declspec(dllimport)
)

if(OPENGL32_USE_TLS)
    add_definitions(-DOPENGL32_USE_TLS)
endif()

if((ARCH STREQUAL "i386") AND (NOT MSVC))
    add_definitions(
        -DUSE_X86_ASM
        -DUSE_MMX_ASM
        -DUSE_3DNOW_ASM
        -DUSE_SSE_ASM)
    add_subdirectory(x86)
endif()

add_subdirectory(drivers/common)
add_subdirectory(main)
add_subdirectory(math)
add_subdirectory(swrast)
add_subdirectory(swrast_setup)
add_subdirectory(tnl)
add_subdirectory(vbo)
