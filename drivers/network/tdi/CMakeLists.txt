
add_definitions(
    -DNDIS40
    -D_NTDRIVER_
    -D_TDI_)

spec2def(tdi.sys misc/tdi.spec)

list(APPEND SOURCE
    cte/events.c
    cte/string.c
    cte/timer.c
    misc/main.c
    tdi/dereg.c
    tdi/handler.c
    tdi/obsolete.c
    tdi/stubs.c
    precomp.h)

add_library(tdi MODULE
    ${SOURCE}
    misc/tdi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/tdi.def)

add_pch(tdi precomp.h SOURCE)
set_module_type(tdi kernelmodedriver)
add_importlibs(tdi ntoskrnl hal)
add_cd_file(TARGET tdi DESTINATION reactos/system32/drivers FOR all)
