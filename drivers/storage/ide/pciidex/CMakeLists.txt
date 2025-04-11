
spec2def(pciidex.sys pciidex.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    fdo.c
    miniport.c
    pciidex.c
    pdo.c
    power.c
    pciidex.h)

add_library(pciidex MODULE
    ${SOURCE}
    pciidex.rc
    ${CMAKE_CURRENT_BINARY_DIR}/pciidex.def)

# add_pch(pciidex pciidex.h SOURCE) TODO
set_module_type(pciidex kernelmodedriver)
add_importlibs(pciidex ntoskrnl hal)
add_cd_file(TARGET pciidex DESTINATION reactos/system32/drivers NO_CAB FOR all)
