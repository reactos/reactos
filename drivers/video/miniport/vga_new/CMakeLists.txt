
list(APPEND SOURCE
    modeset.c
    vgadata.c
    vga.c
    vbemodes.c
    vbe.c
    vga.rc)

add_library(vga SHARED ${CMAKE_CURRENT_BINARY_DIR}/vga_vga.h.gch ${SOURCE})

set_target_properties(vga PROPERTIES COMPILE_FLAGS "-mrtd -fno-builtin" LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native -Wl,--exclude-all-symbols" SUFFIX ".sys")

target_link_libraries(vga libcntpr)

add_importlibs(vga videoprt)
add_pch(vga ${CMAKE_CURRENT_SOURCE_DIR}/vga.h ${SOURCE})
add_dependencies(vga psdk bugcodes)
add_livecd_target(vga reactos/system32/drivers)