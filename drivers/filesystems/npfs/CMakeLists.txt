
list(APPEND SOURCE
    create.c
    finfo.c
    fsctrl.c
    npfs.c
    rw.c
    volume.c
    npfs.rc)

add_library(npfs SHARED ${CMAKE_CURRENT_BINARY_DIR}/npfs_npfs.h.gch ${SOURCE})

set_module_type(npfs kernelmodedriver)
add_importlibs(npfs ntoskrnl hal)

add_pch(npfs ${CMAKE_CURRENT_SOURCE_DIR}/npfs.h ${SOURCE})

add_cab_target(npfs 2)
