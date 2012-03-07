
set_rc_compiler()

spec2def(ole32.dll ole32.spec ADD_IMPORTLIB)
generate_idl_iids(dcom.idl)
add_idl_headers(ole32idl dcom.idl irot.idl)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_OLE32_
    -DENTRY_PREFIX=OLE32_
    -DPROXY_CLSID=CLSID_PSFactoryBuffer
    -DREGISTER_PROXY_DLL
    -DCOM_NO_WINDOWS_H)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
    
add_rpc_files(client irot.idl)

add_rpcproxy_files(
    dcom.idl
    ole32_unknwn.idl
    ole32_objidl.idl
    ole32_oleidl.idl)

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
    ${CMAKE_CURRENT_BINARY_DIR}/dcom_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/dcom_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/irot_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/ole32_unknwn_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/ole32_objidl_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/ole32_oleidl_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/ole32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/ole32.def)

add_library(ole32 SHARED ${SOURCE})

set_module_type(ole32 win32dll)

target_link_libraries(ole32
    wine
    uuid
    ${PSEH_LIB})

add_importlibs(ole32 advapi32 user32 gdi32 rpcrt4 msvcrt kernel32 ntdll)

add_dependencies(ole32 ole32idl)
add_cd_file(TARGET ole32 DESTINATION reactos/system32 FOR all)

