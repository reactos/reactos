
spec2def(framebuf.dll framebuf.spec)

add_library(framebuf SHARED
    enable.c
    palette.c
    pointer.c
    screen.c
    surface.c
    framebuf.rc
    ${CMAKE_CURRENT_BINARY_DIR}/framebuf.def)

set_target_properties(framebuf PROPERTIES LINK_FLAGS "-Wl,-entry,_DrvEnableDriver@12 -Wl,--image-base,0x00010000 -Wl,--subsystem,native")

add_importlibs(framebuf win32k)
add_dependencies(framebuf psdk bugcodes)
add_cab_target(framebuf 1)