
add_definitions(
    -D__WINESRC__
    -DREGISTER_PROXY_DLL)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

generate_idl_iids(qmgrprxy.idl)

spec2def(qmgrprxy.dll qmgrprxy.spec)

list(APPEND SOURCE
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/qmgrprxy_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/qmgrprxy.def)

add_rpcproxy_library(qmgrprxylib qmgrprxy.idl)

add_library(qmgrprxy SHARED ${SOURCE})

set_module_type(qmgrprxy win32dll)
target_link_libraries(qmgrprxy qmgrprxylib ${PSEH_LIB} wine)
add_importlibs(qmgrprxy rpcrt4 msvcrt kernel32 ntdll)
add_cab_target(qmgrprxy 1)
