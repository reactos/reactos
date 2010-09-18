


add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-DUNICODE -D_UNICODE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/dll/win32/dhcpcsvc/include)
include_directories(${REACTOS_SOURCE_DIR}/lib/tdilib)

spec2def(iphlpapi ${CMAKE_CURRENT_SOURCE_DIR}/iphlpapi.spec ${CMAKE_CURRENT_BINARY_DIR}/iphlpapi.def)

add_library(iphlpapi SHARED 
    dhcp_reactos.c
    ifenum_reactos.c
    ipstats_reactos.c
    iphlpapi_main.c
    media.c
    registry.c
    resinfo_reactos.c
    route_reactos.c
    iphlpapi.rc)

set_target_properties(iphlpapi PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(iphlpapi
    ${CMAKE_CURRENT_BINARY_DIR}/iphlpapi.def
    wine
    tdilib
    -ldhcpcsvc
    -ladvapi32
    -lws2_32
    mingw_common
    -lmsvcrt)

add_dependencies(iphlpapi iphlpapi_def psdk)
