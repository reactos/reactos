
include_directories(BEFORE include)

list(APPEND SOURCE
    ne2000/8390.c
    ne2000/main.c
    ne2000.rc)

add_library(ne2000 SHARED ${SOURCE})

set_module_type(ne2000 kernelmodedriver)
add_importlibs(ne2000 ndis ntoskrnl hal)

add_cab_target(ne2000 2)
