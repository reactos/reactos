
include_directories(BEFORE include)
include_directories(${REACTOS_SOURCE_DIR}/lib/drivers/oskittcp/include)

add_definitions(-DNDIS40)
add_definitions(-D_NTDRIVER_)

spec2def(tdi.sys misc/tdi.spec)

list(APPEND SOURCE
    cte/string.c
    cte/stubs.c
    misc/main.c
    misc/tdi.rc
    tdi/dereg.c
    tdi/handler.c
    tdi/obsolete.c
    tdi/stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/tdi.def)

add_library(tdi SHARED ${SOURCE})

set_module_type(tdi kernelmodedriver)

target_link_libraries(tdi
    -lntoskrnl
    -lhal)

add_dependencies(tdi psdk bugcodes buildno_header)

add_cab_target(tdi 2)