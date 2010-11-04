
list(APPEND SOURCE
    initvga.c
    vgamp.c
    vgamp.rc)

add_library(vgamp SHARED ${CMAKE_CURRENT_BINARY_DIR}/vgamp_vgamp.h.gch ${SOURCE})

set_module_type(vgamp kernelmodedriver)
add_importlibs(vgamp videoprt)

add_pch(vgamp ${CMAKE_CURRENT_SOURCE_DIR}/vgamp.h ${SOURCE})
add_cab_target(vgamp 2)
