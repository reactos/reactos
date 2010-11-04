
add_library(serenum SHARED
    detect.c
    fdo.c
    misc.c
    pdo.c
    serenum.c
    serenum.rc)

set_module_type(serenum kernelmodedriver)
add_importlibs(serenum ntoskrnl hal)

add_cab_target(serenum 2)
