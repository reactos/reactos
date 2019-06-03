
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/mbedtls)

add_definitions(-D__WINESRC__ -D_WINE)

list(APPEND SOURCE
    lsamode.c
    schannel_mbedtls.c
    schannel_main.c
    schannel_wine.c
    secur32_wine.c
    usermode.c
    stubs.c
    precomp.h)

add_library(schannel MODULE
    ${SOURCE}
    version.rc)
spec2def(schannel schannel.dll schannel.spec)

set_module_type(schannel win32dll)
target_link_libraries(schannel wine)
add_importlibs(schannel mbedtls crypt32 secur32 advapi32 msvcrt kernel32 ntdll)
add_pch(schannel precomp.h SOURCE)
add_cd_file(TARGET schannel DESTINATION reactos/system32 FOR all)
