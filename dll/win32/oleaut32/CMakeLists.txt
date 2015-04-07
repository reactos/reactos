
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -DCOM_NO_WINDOWS_H
    -D_OLEAUT32_
    -DPROXY_DELEGATION
    -DWINE_REGISTER_DLL
    -DENTRY_PREFIX=OLEAUTPS_
    -DPROXY_CLSID=CLSID_PSFactoryBuffer)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(oleaut32.dll oleaut32.spec ADD_IMPORTLIB)
add_rpcproxy_files(oleaut32_oaidl.idl oleaut32_ocidl.idl)

list(APPEND SOURCE
    connpt.c
    dispatch.c
    hash.c
    oleaut.c
    olefont.c
    olepicture.c
    olepropframe.c
    recinfo.c
    safearray.c
    tmarshal.c
    typelib.c
    usrmarshal.c
    varformat.c
    variant.c
    vartype.c
    precomp.h)

if(MSVC)
    add_asm_files(oleaut32_asm msvc.S)
endif()

add_library(oleaut32 SHARED
    ${SOURCE}
    ${oleaut32_asm}
    oleaut32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/oleaut32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/oleaut32_oaidl_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/oleaut32_ocidl_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/oleaut32.def)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(oleaut32 mingwex)
    #FIXME: http://llvm.org/bugs/show_bug.cgi?id=19027
    set_property(SOURCE vartype.c APPEND_STRING PROPERTY COMPILE_FLAGS " -no-integrated-as")
endif()

add_idl_headers(oleaut32_idlheader oleaut32_oaidl.idl)
add_dependencies(oleaut32 oleaut32_idlheader)
set_module_type(oleaut32 win32dll)
target_link_libraries(oleaut32 wine wineldr uuid ${PSEH_LIB})
add_delay_importlibs(oleaut32 comctl32 urlmon windowscodecs)
add_importlibs(oleaut32 ole32 rpcrt4 user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_pch(oleaut32 precomp.h SOURCE)
add_cd_file(TARGET oleaut32 DESTINATION reactos/system32 FOR all)
