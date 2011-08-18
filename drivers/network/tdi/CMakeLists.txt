
include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/lib/drivers/oskittcp/include)

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
    misc/tdi.rc
    tdi/dereg.c
    tdi/handler.c
    tdi/obsolete.c
    tdi/stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/tdi.def)

add_library(tdi SHARED ${SOURCE})
add_pch(tdi precomp.h)
set_module_type(tdi kernelmodedriver)
add_importlibs(tdi ntoskrnl hal)
add_cd_file(TARGET tdi DESTINATION reactos/system32/drivers FOR all)
