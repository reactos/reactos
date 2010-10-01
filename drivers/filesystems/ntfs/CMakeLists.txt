
list(APPEND SOURCE
    attrib.c
    blockdev.c
    close.c
    create.c
    dirctl.c
    dispatch.c
    fastio.c
    fcb.c
    finfo.c
    fsctl.c
    mft.c
    misc.c
    ntfs.c
    rw.c
    volinfo.c
    ntfs.rc)
    
add_library(ntfs SHARED ${CMAKE_CURRENT_BINARY_DIR}/ntfs_ntfs.h.gch ${SOURCE})

set_target_properties(ntfs PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(ntfs ntoskrnl hal)
add_pch(ntfs ${CMAKE_CURRENT_SOURCE_DIR}/ntfs.h ${SOURCE})
add_dependencies(ntfs psdk bugcodes)
