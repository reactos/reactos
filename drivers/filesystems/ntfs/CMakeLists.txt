
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

set_module_type(ntfs kernelmodedriver)
add_importlibs(ntfs ntoskrnl hal)

add_pch(ntfs ${CMAKE_CURRENT_SOURCE_DIR}/ntfs.h ${SOURCE})

add_cd_file(TARGET ntfs DESTINATION reactos/system32/drivers NO_CAB FOR all)
