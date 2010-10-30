
add_library(serial SHARED
    circularbuffer.c
    cleanup.c
    close.c
    create.c
    devctrl.c
    info.c
    legacy.c
    misc.c
    pnp.c
    power.c
    rw.c
    serial.c
    serial.rc)

set_target_properties(serial PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(serial ntoskrnl hal)
add_dependencies(serial psdk bugcodes)
add_cab_target(serial 2)