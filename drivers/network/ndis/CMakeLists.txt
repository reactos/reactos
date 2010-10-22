
include_directories(BEFORE include)

add_definitions(-DNDIS_WRAPPER)
add_definitions(-DNDIS51)
add_definitions(-DNDIS51_MINIPORT)
add_definitions(-DNDIS_LEGACY_DRIVER)
add_definitions(-DNDIS_LEGACY_MINIPORT)
add_definitions(-DNDIS_LEGACY_PROTOCOL)
add_definitions(-DNDIS_MINIPORT_DRIVER)

spec2def(ndis ${CMAKE_CURRENT_SOURCE_DIR}/ndis.spec ${CMAKE_CURRENT_BINARY_DIR}/ndis.def)

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
    ndis.rc)

add_library(ndis SHARED ${CMAKE_CURRENT_BINARY_DIR}/ndis_ndissys.h.gch ${SOURCE})

set_target_properties(ndis PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(ndis
    ${CMAKE_CURRENT_BINARY_DIR}/ndis.def
    -lntoskrnl
    -lhal)

add_pch(ndis ${CMAKE_CURRENT_SOURCE_DIR}/include/ndissys.h ${SOURCE})
add_dependencies(ndis ndis_def psdk bugcodes buildno_header)
add_livecd_target(ndis reactos/system32/drivers)