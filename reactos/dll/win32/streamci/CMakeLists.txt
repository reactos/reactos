
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(streamci.dll streamci.spec)

list(APPEND SOURCE
    streamci.c
    precomp.h)

add_library(streamci SHARED
    ${SOURCE}
    streamci.rc
    ${CMAKE_CURRENT_BINARY_DIR}/streamci.def)

set_module_type(streamci win32dll)
target_link_libraries(streamci uuid wine)
add_importlibs(streamci rpcrt4 setupapi advapi32 iphlpapi dhcpcsvc ole32 user32 comctl32 ws2_32 msvcrt kernel32 ntdll)
add_pch(streamci precomp.h SOURCE)
add_cd_file(TARGET streamci DESTINATION reactos/system32 FOR all)
