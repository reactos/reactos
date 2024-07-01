
if(ARCH STREQUAL "i386")
    set(DETECT_LEGACY_DEVICES TRUE)
    add_definitions(-DATA_DETECT_LEGACY_DEVICES)
endif()

list(APPEND SOURCE
    ahci.h
    ataacpi.c
    ataacpi.h
    atahw.h
    atapi.c
    atapi.h
    data.c
    debug.h
    enum.c
    fdo.c
    hwahci.c
    hwpata.c
    ioctl.c
    pdo.c
    satl.c
    scsi.c
    smart.c
    timings.c
    wmi.c)

if(DETECT_LEGACY_DEVICES)
    list(APPEND SOURCE legacy.c)
endif()

add_library(atapi MODULE ${SOURCE} atapi.rc)

set_module_type(atapi kernelmodedriver)
target_link_libraries(atapi memcmp ${PSEH_LIB})
# add_pch(atapi atapi.h SOURCE) # TODO
add_importlibs(atapi ntoskrnl hal wmilib)
add_cd_file(TARGET atapi DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(atapi_reg.inf)
