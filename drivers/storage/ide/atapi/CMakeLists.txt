
if(ARCH STREQUAL "i386")
    set(DETECT_LEGACY_DEVICES TRUE)
    add_definitions(-DATA_DETECT_LEGACY_DEVICES)
endif()

list(APPEND SOURCE
    include/acpi.h
    include/ahci.h
    include/atahw.h
    include/debug.h
    include/devevent.h
    include/identify_funcs.h
    include/pata.h
    include/request.h
    include/scsiex.h
    acpi.c
    ahci_hw.c
    ahci_io.c
    ahci_mem.c
    atapi.c
    atapi.h
    data.c
    dev_config.c
    dev_error.c
    dev_identify.c
    dev_timings.c
    enum.c
    fdo.c
    ioctl.c
    pata_hw.c
    pata_io.c
    pdo.c
    portstate.c
    satl.c
    scsi.c
    smart.c
    power.c
    wmi.c)

if(DETECT_LEGACY_DEVICES)
    list(APPEND SOURCE pata_legacy.c)
endif()

add_library(atapi MODULE ${SOURCE} atapi.rc)

set_module_type(atapi kernelmodedriver)
target_link_libraries(atapi memcmp ${PSEH_LIB})
# add_pch(atapi atapi.h SOURCE) # TODO
add_importlibs(atapi ntoskrnl hal wmilib)
add_cd_file(TARGET atapi DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(atapi_reg.inf)
