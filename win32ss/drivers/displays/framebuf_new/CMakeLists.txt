
list(APPEND SOURCE
    debug.c
    enable.c
    palette.c
    pointer.c
    screen.c
    driver.h)

add_library(framebuf MODULE
    ${SOURCE}
    framebuf.rc)

set_module_type(framebuf kerneldll ENTRYPOINT DrvEnableDriver 12)
target_link_libraries(framebuf libcntpr)
add_importlibs(framebuf win32k)
add_pch(framebuf driver.h SOURCE)
add_dependencies(framebuf psdk)
add_cd_file(TARGET framebuf DESTINATION reactos/system32 FOR all)
