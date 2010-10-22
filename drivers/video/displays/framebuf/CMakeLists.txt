
spec2def(framebuf ${CMAKE_CURRENT_SOURCE_DIR}/framebuf.spec ${CMAKE_CURRENT_BINARY_DIR}/framebuf.def)

add_library(framebuf SHARED
    enable.c
    palette.c
    pointer.c
    screen.c
    surface.c
    framebuf.rc)

set_target_properties(framebuf PROPERTIES LINK_FLAGS "-Wl,-entry,_DrvEnableDriver@12 -Wl,--image-base,0x00010000 -Wl,--subsystem,native")

target_link_libraries(framebuf
    ${CMAKE_CURRENT_BINARY_DIR}/framebuf.def)

add_importlibs(framebuf win32k)
add_dependencies(framebuf framebuf_def psdk bugcodes)
add_livecd_target(framebuf reactos/system32)