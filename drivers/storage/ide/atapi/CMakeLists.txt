
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/sptilib)

list(APPEND SOURCE
    atapi.c
    atapi.h
    debug.h
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
    scsiex.h
    smart.c
    wmi.c)

add_library(atapi MODULE ${SOURCE} atapi.rc)

if (STACK_PROTECTOR)
    target_sources(atapi PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

set_module_type(atapi kernelmodedriver)
target_link_libraries(atapi memcmp sptilib ${PSEH_LIB})
add_pch(atapi atapi.h SOURCE)
add_importlibs(atapi ntoskrnl hal wmilib)
add_cd_file(TARGET atapi DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(atapi_reg.inf)
