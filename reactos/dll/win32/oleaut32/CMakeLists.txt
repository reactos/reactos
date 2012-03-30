
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-DPROXY_CLSID_IS="{0xb196b286,0xbab4,0x101a,{0xb6,0x9c,0x00,0xaa,0x00,0x34,0x1d,0x07}}")

add_definitions(
    -D__WINESRC__
    -DCOM_NO_WINDOWS_H
    -D_OLEAUT32_
    -DPROXY_DELEGATION
    -DREGISTER_PROXY_DLL
    -DENTRY_PREFIX=OLEAUTPS_)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/libjpeg
    ${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(oleaut32.dll oleaut32.spec ADD_IMPORTLIB)

add_rpcproxy_files(oleaut32_oaidl.idl oleaut32_ocidl.idl)

list(APPEND SOURCE
    connpt.c
    dispatch.c
    hash.c
    oleaut.c
    olefont.c
    olepicture.c
    recinfo.c
    regsvr.c
    safearray.c
    stubs.c
    tmarshal.c
    typelib.c
    typelib2.c
    ungif.c
    usrmarshal.c
    varformat.c
    variant.c
    vartype.c
    oleaut32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/oleaut32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/oleaut32_oaidl_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/oleaut32_ocidl_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/oleaut32.def)

if(MSVC)
    list(APPEND SOURCE msvc.S)
endif()

add_library(oleaut32 SHARED ${SOURCE})
set_module_type(oleaut32 win32dll)

target_link_libraries(oleaut32
    wine
    wineldr
    uuid
    ${PSEH_LIB})

add_delay_importlibs(oleaut32 comctl32 urlmon windowscodecs)
add_importlibs(oleaut32 ole32 rpcrt4 user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET oleaut32 DESTINATION reactos/system32 FOR all)

