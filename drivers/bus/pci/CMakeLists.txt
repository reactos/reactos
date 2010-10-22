
add_library(pci SHARED
    fdo.c
    pci.c
    pdo.c
    pci.rc)

set_target_properties(pci PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native -Wl,--exclude-all-symbols" SUFFIX ".sys")

add_importlibs(pci ntoskrnl hal)
add_dependencies(pci psdk bugcodes buildno_header)
add_minicd_target(pci reactos pci.sys)
add_livecd_target(pci reactos/system32/drivers)