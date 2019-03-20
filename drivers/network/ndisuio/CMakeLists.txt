
add_definitions(
    -DNDIS50
    -D_NTDRIVER_)

include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers/ndisuio)

list(APPEND SOURCE
    createclose.c
    ioctl.c
    main.c
    misc.c
    protocol.c
    readwrite.c
    ndisuio.h)

add_library(ndisuio MODULE ${SOURCE} ndisuio.rc)
set_module_type(ndisuio kernelmodedriver)
add_importlibs(ndisuio ndis ntoskrnl hal)
add_pch(ndisuio ndisuio.h SOURCE)
add_cd_file(TARGET ndisuio DESTINATION reactos/system32/drivers FOR all)
