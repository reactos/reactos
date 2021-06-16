spec2def(wdfldr.sys wdfldr.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    class.c
    common.c
    library.c
    registry.c
    wdfldr.c)

add_library(wdfldr MODULE
    ${SOURCE}
    wdfldr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wdfldr.def)

set_module_type(wdfldr kernelmodedriver ENTRYPOINT 0)
target_link_libraries(wdfldr aux_klib libcntpr ${PSEH_LIB})
add_importlibs(wdfldr ntoskrnl hal)
add_pch(wdfldr wdfloader.h SOURCE)
add_cd_file(TARGET wdfldr DESTINATION reactos/system32/drivers NO_CAB FOR all)

target_include_directories(wdfldr
    PRIVATE ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/wdf/shared/inc/private/common
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/wdf/kmdf/inc/private
)
