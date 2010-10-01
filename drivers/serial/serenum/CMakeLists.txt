
add_library(serenum SHARED
    detect.c
    fdo.c
    misc.c
    pdo.c
    serenum.c
    serenum.rc)

set_target_properties(serenum PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(serenum ntoskrnl hal)
add_dependencies(serenum psdk bugcodes)
