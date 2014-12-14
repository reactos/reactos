
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

list(APPEND SOURCE
    condrv.c
    control.c
    dispatch.c
    condrv.h)

add_library(condrv SHARED
    ${SOURCE}
    condrv.rc)

set_module_type(condrv kernelmodedriver)
target_link_libraries(condrv ${PSEH_LIB})
add_importlibs(condrv ntoskrnl hal)
add_pch(condrv condrv.h SOURCE)
add_cd_file(TARGET condrv DESTINATION reactos/system32/drivers NO_CAB FOR all)
