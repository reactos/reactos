
spec2def(framebuf_new.dll framebuf_new.spec)

list(APPEND SOURCE
    debug.c
    enable.c
    palette.c
    pointer.c
    screen.c
    driver.h)

add_library(framebuf_new SHARED
    ${SOURCE}
    framebuf_new.rc
    ${CMAKE_CURRENT_BINARY_DIR}/framebuf_new.def)

set_module_type(framebuf_new kerneldll ENTRYPOINT DrvEnableDriver 12)
target_link_libraries(framebuf_new libcntpr)
add_importlibs(framebuf_new win32k)
add_pch(framebuf_new driver.h SOURCE)
add_dependencies(framebuf_new psdk)
