
list(APPEND SOURCE
    initvga.c
    vgamp.c
    vgamp.rc)

add_library(vgamp SHARED ${CMAKE_CURRENT_BINARY_DIR}/vgamp_vgamp.h.gch ${SOURCE})

set_target_properties(vgamp PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native -Wl,--exclude-all-symbols" SUFFIX ".sys")

add_importlibs(vgamp videoprt)
add_pch(vgamp ${CMAKE_CURRENT_SOURCE_DIR}/vgamp.h ${SOURCE})
add_dependencies(vgamp psdk bugcodes)
add_livecd_target(vgamp reactos/system32/drivers)