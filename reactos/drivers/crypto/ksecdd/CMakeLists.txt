
spec2def(ksecdd.sys ksecdd.spec)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers/ksecdd
    ${REACTOS_SOURCE_DIR}/lib/cryptlib)

list(APPEND SOURCE
    ksecdd.c
    dispatch.c
    crypt.c
    random.c
    stubs.c
    ksecdd.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ksecdd.def)

add_library(ksecdd SHARED ${SOURCE})
target_link_libraries(ksecdd cryptlib pseh)
set_module_type(ksecdd kernelmodedriver)
add_importlibs(ksecdd ntoskrnl hal)
add_cd_file(TARGET ksecdd DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(ksecdd_reg.inf)
