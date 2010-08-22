
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(include)
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/adns/src)
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/adns/adns_win32)

add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-DADNS_JGAA_WIN32)

spec2def(dnsapi ${CMAKE_CURRENT_SOURCE_DIR}/dnsapi.spec ${CMAKE_CURRENT_BINARY_DIR}/dnsapi.def)

list(APPEND SOURCE 
    dnsapi/adns.c
    dnsapi/context.c
    dnsapi/memory.c
    dnsapi/names.c
    dnsapi/query.c
    dnsapi/record.c
    dnsapi/stubs.c
    dnsapi.rc)

add_library(dnsapi SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/dnsapi_precomp.h.gch
    ${SOURCE})

set_target_properties(dnsapi PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(dnsapi
    ${CMAKE_CURRENT_BINARY_DIR}/dnsapi.def
    adns
    ${REACTOS_SOURCE_DIR}/dll/win32/user32/libuser32.a
    -lws2_32
    ${REACTOS_SOURCE_DIR}/dll/win32/msvcrt/libmsvcrt.a
    -liphlpapi
    ${REACTOS_SOURCE_DIR}/dll/win32/kernel32/libkernel32.a
    ${REACTOS_SOURCE_DIR}/dll/ntdll/libntdll.a)

add_pch(dnsapi ${CMAKE_CURRENT_SOURCE_DIR}/dnsapi/precomp.h ${SOURCE})
add_dependencies(dnsapi dnsapi_def psdk)
