
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(actxprxy ${CMAKE_CURRENT_SOURCE_DIR}/actxprxy.spec ${CMAKE_CURRENT_BINARY_DIR}/actxprxy.def)

add_library(actxprxy SHARED usrmarshal.c)

set_entrypoint(actxprxy 0)

target_link_libraries(actxprxy
    ${CMAKE_CURRENT_BINARY_DIR}/actxprxy.def
    actxprxy_proxy
    uuid
    wine
    pseh)

add_importlibs(actxprxy
    rpcrt4
    ole32
    oleaut32
    kernel32
    ntdll)

add_dependencies(actxprxy actxprxy_def)

add_definitions(-DREGISTER_PROXY_DLL -DPROXY_DELEGATION)

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

add_cab_target(actxprxy 1)
