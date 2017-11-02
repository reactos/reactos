
# include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

list(APPEND SOURCE
    dispatch.c
    setupdd.c
    setupdd.h
)

add_library(setupdd SHARED ${SOURCE} setupdd.rc)

set_module_type(setupdd kernelmodedriver)
# target_link_libraries(setupdd ${PSEH_LIB})
add_importlibs(setupdd ntoskrnl hal)
add_pch(setupdd setupdd.h SOURCE)
add_cd_file(TARGET setupdd DESTINATION reactos/system32 NO_CAB FOR bootcd regtest)
