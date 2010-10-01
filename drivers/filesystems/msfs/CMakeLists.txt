
list(APPEND SOURCE
    create.c
    finfo.c
    fsctrl.c
    msfs.c
    rw.c
    msfs.rc)

add_library(msfs SHARED ${CMAKE_CURRENT_BINARY_DIR}/msfs_msfs.h.gch ${SOURCE})

set_target_properties(msfs PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(msfs ntoskrnl hal)
add_pch(msfs ${CMAKE_CURRENT_SOURCE_DIR}/msfs.h ${SOURCE})
add_dependencies(msfs psdk bugcodes)
