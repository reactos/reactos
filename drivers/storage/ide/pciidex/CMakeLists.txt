
spec2def(pciidex.sys pciidex.spec)

add_library(pciidex SHARED
    fdo.c
    miniport.c
    misc.c
    pciidex.c
    pdo.c
    pciidex.rc
    ${CMAKE_CURRENT_BINARY_DIR}/pciidex.def)

set_target_properties(pciidex PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(pciidex ntoskrnl)
add_dependencies(pciidex psdk bugcodes)
add_cab_target(pciidex 2)
