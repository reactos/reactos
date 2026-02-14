
spec2def(scsiport.sys scsiport.spec ADD_IMPORTLIB)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/sptilib)

# Link GCC stack protector lib
if (STACK_PROTECTOR)
    target_link_libraries(libscsiport INTERFACE gcc_ssp_scsiport)
endif()

list(APPEND SOURCE
    fdo.c
    ioctl.c
    pdo.c
    power.c
    registry.c
    scsi.c
    scsiport.c
    stubs.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(scsiport MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    scsiport.rc
    ${CMAKE_CURRENT_BINARY_DIR}/scsiport.def)

add_pch(scsiport scsiport.h "${PCH_SKIP_SOURCE}")
set_module_type(scsiport kernelmodedriver)
target_link_libraries(scsiport sptilib ${PSEH_LIB})
add_importlibs(scsiport ntoskrnl hal)
add_cd_file(TARGET scsiport DESTINATION reactos/system32/drivers NO_CAB FOR all)
