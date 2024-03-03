
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/sptilib)

if(ARCH STREQUAL "i386")
    set(DETECT_LEGACY_DEVICES TRUE)
    add_definitions(-DATA_DETECT_LEGACY_DEVICES)
endif()

list(APPEND SOURCE
    include/atahw.h
    include/debug.h
    include/devevent.h
    include/scsiex.h
    atapi.c
    atapi.h
    data.c
    dev_config.c
    dev_error.c
    dev_identify.c
    dev_power.c
    dev_timings.c
    enum.c
    fdo.c
    ioctl.c
    pdo.c
    portstate.c
    satl.c
    scsi.c
    smart.c
    wmi.c)

if(DETECT_LEGACY_DEVICES)
    list(APPEND SOURCE pata_legacy.c)
endif()

add_library(atapi MODULE ${SOURCE} atapi.rc)

if (STACK_PROTECTOR)
    target_sources(atapi PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

set_module_type(atapi kernelmodedriver)
target_link_libraries(atapi memcmp sptilib ${PSEH_LIB})
# add_pch(atapi atapi.h SOURCE) # TODO
add_importlibs(atapi ntoskrnl hal wmilib)
add_cd_file(TARGET atapi DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(atapi_reg.inf)
