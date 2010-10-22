
add_library(pcmcia SHARED
    fdo.c
    pcmcia.c
    pdo.c
    pcmcia.rc)

set_target_properties(pcmcia PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native -Wl,--exclude-all-symbols" SUFFIX ".sys")

add_importlibs(pcmcia ntoskrnl hal)
add_dependencies(pcmcia psdk bugcodes buildno_header)
add_minicd_target(pcmcia reactos pcmcia.sys)
add_livecd_target(pcmcia reactos/system32/drivers)