
spec2def(framebuf_new.dll framebuf_new.spec)

list(APPEND SOURCE
    debug.c
    enable.c
    palette.c
    pointer.c
    screen.c
    framebuf_new.rc
    ${CMAKE_CURRENT_BINARY_DIR}/framebuf_new.def)

add_library(framebuf_new SHARED ${CMAKE_CURRENT_BINARY_DIR}/framebuf_new_driver.h.gch ${SOURCE})

if(ARCH MATCHES i386)
    if(MSVC)
    else()
        set_target_properties(framebuf_new PROPERTIES COMPILE_FLAGS "-mrtd -fno-builtin")
    endif()
endif()

set_target_properties(framebuf_new PROPERTIES LINK_FLAGS "-Wl,-entry,_DrvEnableDriver@12 -Wl,--image-base,0x00010000 -Wl,--subsystem,native")

target_link_libraries(framebuf_new libcntpr)

add_importlibs(framebuf_new win32k)
add_pch(framebuf_new ${CMAKE_CURRENT_SOURCE_DIR}/driver.h ${SOURCE})
add_dependencies(framebuf_new psdk bugcodes)
