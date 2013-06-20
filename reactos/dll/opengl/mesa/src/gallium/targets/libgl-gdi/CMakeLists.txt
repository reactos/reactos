
# we link against C++ static libs
set_cpp()

include_directories(
    ../../state_trackers/wgl
    ../../winsys/sw
)

add_definitions(-DWIN32_LEAN_AND_MEAN)

list(APPEND SOURCE
    libgl_gdi.c)

# We don't need a specfile here, as opengl application link against opengl32
# and gallium is kind enough to provide good def files for both MSVC/mingw-w64 and mingw32
if(MSVC OR (ARCH STREQUAL "amd64"))
    list(APPEND SOURCE ../../state_trackers/wgl/opengl32.def)
else()    
    list(APPEND SOURCE ../../state_trackers/wgl/opengl32.mingw.def)
endif()

add_library(mesa32 SHARED ${SOURCE})

target_link_libraries(mesa32 
    gallium_wgl
    gallium_ws_gdi
    mesa_glapi
    mesa_core
    gallium_softpipe
    gallium_core
    mesa_glsl
)

set_module_type(mesa32 win32dll)

add_importlibs(mesa32 gdi32 user32 msvcrt ws2_32 kernel32 ntdll)

if(MSVC)
    target_link_libraries(mesa32 oldnames)
endif()

add_dependencies(mesa32 psdk)
add_cd_file(TARGET mesa32 DESTINATION reactos/system32 FOR all)
    