
list(APPEND SOURCE
    antimoniker.c
    bindctx.c
    classmoniker.c
    clipboard.c
    comcat.c
    compobj.c
    compositemoniker.c
    datacache.c
    defaulthandler.c
    dictionary.c
    enumx.c
    errorinfo.c
    filelockbytes.c
    filemoniker.c
    ftmarshal.c
    git.c
    hglobalstream.c
    ifs.c
    itemmoniker.c
    marshal.c
    memlockbytes.c
    moniker.c
    ole2.c
    ole2stubs.c
    ole2impl.c
    ole32_main.c
    oleobj.c
    oleproxy.c
    pointermoniker.c
    regsvr.c
    rpc.c
    stg_prop.c
    stg_stream.c
    storage32.c
    stubmanager.c
    usrmarshal.c
    ole32res.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ole32.def)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_OLE32_
    -DENTRY_PREFIX=OLE32_
    -DPROXY_CLSID=CLSID_PSFactoryBuffer
    -DREGISTER_PROXY_DLL
    -DCOM_NO_WINDOWS_H)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${REACTOS_SOURCE_DIR}/include/reactos/wine)

ADD_INTERFACE_DEFINITIONS(ole32idl dcom.idl irot.idl)
rpcproxy(ole32
    dcom.idl
    ole32_unknwn.idl
    ole32_objidl.idl
    ole32_oleidl.idl)
    
spec2def(ole32.dll ole32.spec)

add_library(ole32 SHARED ${SOURCE})

set_module_type(ole32 win32dll)

target_link_libraries(ole32
    wine
    irot_client
    ole32_proxy
    uuid
    ${PSEH_LIB})
    
add_importlibs(ole32 advapi32 user32 gdi32 rpcrt4 msvcrt kernel32 ntdll)
    
add_dependencies(ole32 ole32idl)
add_cab_target(ole32 1)
add_importlib_target(ole32.spec)
