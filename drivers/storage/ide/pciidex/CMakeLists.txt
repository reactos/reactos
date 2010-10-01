
spec2def(pciidex ${CMAKE_CURRENT_SOURCE_DIR}/pciidex.spec ${CMAKE_CURRENT_BINARY_DIR}/pciidex.def)

add_library(pciidex SHARED
    fdo.c
    miniport.c
    misc.c
    pciidex.c
    pdo.c
    pciidex.rc)

set_target_properties(pciidex PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(pciidex
    ${CMAKE_CURRENT_BINARY_DIR}/pciidex.def)

add_importlibs(pciidex ntoskrnl)
add_dependencies(pciidex pciidex_def psdk bugcodes)
