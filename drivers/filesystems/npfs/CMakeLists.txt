
list(APPEND SOURCE
    create.c
    finfo.c
    fsctrl.c
    npfs.c
    rw.c
    volume.c
    npfs.rc)

add_library(npfs SHARED ${CMAKE_CURRENT_BINARY_DIR}/npfs_npfs.h.gch ${SOURCE})

set_target_properties(npfs PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(npfs ntoskrnl hal)

add_pch(npfs ${CMAKE_CURRENT_SOURCE_DIR}/npfs.h ${SOURCE})
add_dependencies(npfs psdk bugcodes)
add_livecd_target(npfs reactos/system32/drivers)