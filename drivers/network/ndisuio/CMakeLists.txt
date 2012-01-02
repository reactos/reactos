add_definitions(
    -DNDIS50
    -D_NTDRIVER_)

spec2def(ndisuio.sys ndisuio.spec)

list(APPEND SOURCE
    main.c
    ndisuio.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ndisuio.def)

add_library(ndisuio SHARED ${SOURCE})

target_link_libraries(ndisuio
    ${PSEH_LIB})

set_module_type(ndisuio kernelmodedriver)
add_importlibs(ndisuio ndis ntoskrnl hal)

add_pch(ndisuio ndisuio.h)

add_cd_file(TARGET ndisuio DESTINATION reactos/system32/drivers FOR all)
