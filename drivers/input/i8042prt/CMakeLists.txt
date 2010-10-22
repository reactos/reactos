


add_library(i8042prt SHARED
    createclose.c
    i8042prt.c
    keyboard.c
    misc.c
    mouse.c
    pnp.c
    ps2pp.c
    readwrite.c
    registry.c
    setup.c
    i8042prt.rc)

set_target_properties(i8042prt PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols" SUFFIX ".sys")

target_link_libraries(i8042prt
    -lntoskrnl
    -lhal)

add_dependencies(i8042prt psdk bugcodes)
add_minicd_target(i8042prt reactos i8042prt.sys)
add_livecd_target(i8042prt reactos/system32/drivers)