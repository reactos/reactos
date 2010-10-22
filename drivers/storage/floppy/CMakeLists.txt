


add_library(floppy SHARED
    csqrtns.c
    floppy.c
    hardware.c
    ioctl.c
    readwrite.c
    floppy.rc)

set_target_properties(floppy PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(floppy
    csq
    -lntoskrnl
    -lhal)

add_dependencies(floppy psdk bugcodes buildno_header)
add_minicd_target(floppy reactos floppy.sys)
add_livecd_target(floppy reactos/system32/drivers)