
spec2def(framebuf_new.dll framebuf_new.spec)

list(APPEND SOURCE
    debug.c
    enable.c
    palette.c
    pointer.c
    screen.c
    framebuf_new.rc
    ${CMAKE_CURRENT_BINARY_DIR}/framebuf_new.def)

add_library(framebuf_new SHARED ${SOURCE})

if(ARCH MATCHES i386)
    if(MSVC)
        set_target_properties(framebuf_new PROPERTIES COMPILE_FLAGS "/Gz")
    else()
        set_target_properties(framebuf_new PROPERTIES COMPILE_FLAGS "-mrtd -fno-builtin")
    endif()
endif()

set_entrypoint(framebuf_new DrvEnableDriver@12)
set_subsystem(framebuf_new native)
set_image_base(framebuf_new 0x00010000)

target_link_libraries(framebuf_new libcntpr)

add_importlibs(framebuf_new win32k)
add_pch(framebuf_new driver.h)
add_dependencies(framebuf_new psdk bugcodes)
