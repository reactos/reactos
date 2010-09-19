
include_directories(BEFORE include)
include_directories(${REACTOS_SOURCE_DIR}/lib/drivers/oskittcp/include)

add_definitions(-DNDIS40)
add_definitions(-D_NTDRIVER_)

spec2def(tdi ${CMAKE_CURRENT_SOURCE_DIR}/misc/tdi.spec ${CMAKE_CURRENT_BINARY_DIR}/tdi.def)

list(APPEND SOURCE
    cte/string.c
    cte/stubs.c
    misc/main.c
    misc/tdi.rc
    tdi/dereg.c
    tdi/handler.c
    tdi/obsolete.c
    tdi/stubs.c)

add_library(tdi SHARED ${SOURCE})

set_target_properties(tdi PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(tdi
    ${CMAKE_CURRENT_BINARY_DIR}/tdi.def
    -lntoskrnl
    -lhal)

add_dependencies(tdi tdi_def psdk bugcodes buildno_header)
