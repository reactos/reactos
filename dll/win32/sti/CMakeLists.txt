
add_definitions(
    -D__WINESRC__
    -DENTRY_PREFIX=STI_
    -DPROXY_DELEGATION
    -DWINE_REGISTER_DLL)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(sti.dll sti.spec)
add_rpcproxy_files(sti_wia.idl)

list(APPEND SOURCE
    sti.c
    sti_main.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/sti_wia_p.c)

add_library(sti MODULE
    ${SOURCE}
    guid.c
    sti.rc
    ${CMAKE_CURRENT_BINARY_DIR}/sti.def)

set_module_type(sti win32dll)
target_link_libraries(sti wine uuid ${PSEH_LIB})
add_importlibs(sti ole32 oleaut32 rpcrt4 advapi32 msvcrt kernel32 ntdll)
add_pch(sti precomp.h SOURCE)
add_cd_file(TARGET sti DESTINATION reactos/system32 FOR all)
