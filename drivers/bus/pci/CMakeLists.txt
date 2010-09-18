


add_library(pci SHARED
    fdo.c
    pci.c
    pdo.c
    pci.rc)

set_target_properties(pci PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(pci
    -lntoskrnl
    -lhal)

add_dependencies(pci psdk bugcodes buildno_header)
