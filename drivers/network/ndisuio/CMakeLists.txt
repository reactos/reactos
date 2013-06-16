add_definitions(
    -DNDIS50
    -D_NTDRIVER_)

include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers/ndisuio)

list(APPEND SOURCE
     createclose.c
     ioctl.c
     main.c
     misc.c
     protocol.c
     readwrite.c
     ndisuio.rc)

add_library(ndisuio SHARED ${SOURCE})

set_module_type(ndisuio kernelmodedriver)
add_importlibs(ndisuio ndis ntoskrnl hal)

add_pch(ndisuio ndisuio.h)

add_cd_file(TARGET ndisuio DESTINATION reactos/system32/drivers FOR all)
