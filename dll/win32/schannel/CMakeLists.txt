
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(schannel.dll schannel.spec)

list(APPEND SOURCE
    lsamode.c
    schannel_main.c
    usermode.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/schannel_stubs.c)

add_library(schannel SHARED
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/schannel.def)

set_module_type(schannel win32dll)
target_link_libraries(schannel wine)
add_importlibs(schannel secur32 msvcrt kernel32 ntdll)
add_pch(schannel precomp.h SOURCE)
add_cd_file(TARGET schannel DESTINATION reactos/system32 FOR all)
