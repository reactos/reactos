
spec2def(netio.sys netio.spec ADD_IMPORTLIB)

# TODO: dependency on afd.h should go away ...

include_directories(
    BEFORE include
    ../afd/include
    ../tdihelpers/include
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

list(APPEND SOURCE
    netio.c
    ../tdihelpers/tdi.c
    ../tdihelpers/tdiconn.c
    ${CMAKE_CURRENT_BINARY_DIR}/netio.def
    )

add_library(netio MODULE ${SOURCE})
target_link_libraries(netio ${PSEH_LIB})
set_module_type(netio kernelmodedriver)
add_importlibs(netio ntoskrnl hal)
add_cd_file(TARGET netio DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(netio_reg.inf)
