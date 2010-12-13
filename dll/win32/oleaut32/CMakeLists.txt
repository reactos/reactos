
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
    oleaut32_oaidl.idl
    ${CMAKE_CURRENT_BINARY_DIR}/oleaut32.def)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

#FIXME: this should be quotes enclosed
add_definitions(-DPROXY_CLSID_IS={0xb196b286,0xbab4,0x101a,{0xb6,0x9c,0x00,0xaa,0x00,0x34,0x1d,0x07}})
#add_definitions(-DPROXY_CLSID_IS="{0xb196b286,0xbab4,0x101a,{0xb6,0x9c,0x00,0xaa,0x00,0x34,0x1d,0x07}}")

add_definitions(
    -D__WINESRC__
    -DCOM_NO_WINDOWS_H
    -D_OLEAUT32_
    -DPROXY_DELEGATION
    -DREGISTER_PROXY_DLL
    -DENTRY_PREFIX=OLEAUTPS_)

if(MSC)
# seems to work, but is this correct ?
# <compilerflag compilerset="msc">/FIwine/typeof.h</compilerflag>
add_definitions(/FIwine/typeof.h)
endif(MSC)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/libjpeg
    ${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(oleaut32.dll oleaut32.spec)

add_library(oleaut32 SHARED ${SOURCE})

set_module_type(oleaut32 win32dll)

target_link_libraries(oleaut32
    oleaut32_proxy
    wine
    wineldr
    uuid
    ${PSEH_LIB})

add_importlibs(oleaut32 windowscodecs ole32 rpcrt4 user32 gdi32 advapi32 comctl32 urlmon msvcrt kernel32 ntdll)

rpcproxy(oleaut32 oleaut32_oaidl.idl oleaut32_ocidl.idl)

add_cab_target(oleaut32 1)
add_importlib_target(oleaut32.spec)
