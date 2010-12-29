
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(actxprxy.dll actxprxy.spec)

rpcproxy(actxprxy
    actxprxy_activscp.idl
    actxprxy_comcat.idl
    actxprxy_docobj.idl
    actxprxy_hlink.idl
    actxprxy_htiframe.idl
    actxprxy_objsafe.idl
    actxprxy_ocmm.idl
    actxprxy_servprov.idl
    actxprxy_shobjidl.idl
    actxprxy_urlhist.idl)

list(APPEND SOURCE
    usrmarshal.c
    ${CMAKE_CURRENT_BINARY_DIR}/actxprxy.def)

add_library(actxprxy SHARED ${SOURCE})

set_entrypoint(actxprxy 0)

target_link_libraries(actxprxy
    uuid
    wine
    ${PSEH_LIB})

add_importlibs(actxprxy
    rpcrt4
    ole32
    oleaut32
    kernel32
    ntdll)

add_definitions(-DREGISTER_PROXY_DLL -DPROXY_DELEGATION)

add_cab_target(actxprxy 1)
