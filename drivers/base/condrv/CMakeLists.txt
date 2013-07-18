
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

## spec2def(condrv.sys condrv.spec ADD_IMPORTLIB)

add_library(condrv SHARED
    condrv.c
    control.c
    condrv.rc)
    # ${CMAKE_CURRENT_BINARY_DIR}/condrv.def)

add_pch(condrv condrv.h)

set_module_type(condrv kernelmodedriver)
target_link_libraries(condrv ${PSEH_LIB})
add_importlibs(condrv ntoskrnl hal)

add_cd_file(TARGET condrv DESTINATION reactos/system32/drivers NO_CAB FOR all)
