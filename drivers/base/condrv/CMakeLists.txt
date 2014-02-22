
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

## spec2def(condrv.sys condrv.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    condrv.c
    control.c
    condrv.h)

add_library(condrv SHARED
    ${SOURCE}
    condrv.rc)
    # ${CMAKE_CURRENT_BINARY_DIR}/condrv.def)

add_pch(condrv condrv.h SOURCE)

set_module_type(condrv kernelmodedriver)
target_link_libraries(condrv ${PSEH_LIB})
add_importlibs(condrv ntoskrnl hal)

add_cd_file(TARGET condrv DESTINATION reactos/system32/drivers NO_CAB FOR all)
