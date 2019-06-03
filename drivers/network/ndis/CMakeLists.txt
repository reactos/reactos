
include_directories(BEFORE include)

add_definitions(
    -DNDIS_WRAPPER
    -DNDIS51
    -DNDIS51_MINIPORT
    -DNDIS_LEGACY_DRIVER
    -DNDIS_LEGACY_MINIPORT
    -DNDIS_LEGACY_PROTOCOL
    -DNDIS_MINIPORT_DRIVER)

list(APPEND SOURCE
    ndis/30stubs.c
    ndis/40stubs.c
    ndis/50stubs.c
    ndis/buffer.c
    ndis/cl.c
    ndis/cm.c
    ndis/co.c
    ndis/config.c
    ndis/control.c
    ndis/efilter.c
    ndis/hardware.c
    ndis/io.c
    ndis/main.c
    ndis/memory.c
    ndis/miniport.c
    ndis/misc.c
    ndis/protocol.c
    ndis/string.c
    ndis/time.c
    include/ndissys.h)

add_library(ndis MODULE
    ${SOURCE}
    ndis.rc)
spec2def(ndis ndis.sys ndis.spec ADD_IMPORTLIB)

set_module_type(ndis kernelmodedriver)
add_importlibs(ndis ntoskrnl hal)
add_pch(ndis include/ndissys.h SOURCE)
add_cd_file(TARGET ndis DESTINATION reactos/system32/drivers FOR all)
