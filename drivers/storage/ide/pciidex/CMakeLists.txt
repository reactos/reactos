
spec2def(pciidex.sys pciidex.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    chipset/ahci_generic.c
    chipset/ahci_hw.c
    chipset/ahci_io.c
    chipset/amd.c
    chipset/ati.c
    chipset/cmd.c
    chipset/intel.c
    chipset/pata_generic.c
    chipset/pata_hw.c
    chipset/pata_io.c
    chipset/pctech.c
    chipset/serverworks.c
    chipset/toshiba.c
    chipset/via.c
    acpi.c
    ahci.h
    debug.h
    fdo.c
    miniport.c
    pciidex.c
    pciidex.h
    pdo.c
    power.c)

add_library(pciidex MODULE
    ${SOURCE}
    pciidex.rc
    ${CMAKE_CURRENT_BINARY_DIR}/pciidex.def)

# add_pch(pciidex pciidex.h SOURCE) TODO
set_module_type(pciidex kernelmodedriver)
add_importlibs(pciidex ntoskrnl hal)
add_cd_file(TARGET pciidex DESTINATION reactos/system32/drivers NO_CAB FOR all)
