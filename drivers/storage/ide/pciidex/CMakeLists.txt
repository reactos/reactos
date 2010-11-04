
spec2def(pciidex.sys pciidex.spec)

add_library(pciidex SHARED
    fdo.c
    miniport.c
    misc.c
    pciidex.c
    pdo.c
    pciidex.rc
    ${CMAKE_CURRENT_BINARY_DIR}/pciidex.def)

set_module_type(pciidex kernelmodedriver)
add_importlibs(pciidex ntoskrnl)

add_cab_target(pciidex 2)
