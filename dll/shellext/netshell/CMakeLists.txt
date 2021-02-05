
project(SHELL)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D_NETSHELL_)
spec2def(netshell.dll netshell.spec)

list(APPEND SOURCE
    netshell.cpp
    shfldr_netconnect.cpp
    enumlist.cpp
    connectmanager.cpp
    lanconnectui.cpp
    lanstatusui.cpp
    setup.cpp)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/netshell_stubs.c)

file(GLOB netshell_rc_deps res/*.*)
add_rc_deps(netshell.rc ${netshell_rc_deps})

add_library(netshell MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    netshell.rc
    ${CMAKE_CURRENT_BINARY_DIR}/netshell.def)

set_module_type(netshell win32dll UNICODE)
target_link_libraries(netshell uuid wine cpprt atl_classes)
add_delay_importlibs(netshell ole32 oleaut32 shlwapi shell32)
add_importlibs(netshell version iphlpapi gdi32 user32 advapi32 setupapi ws2_32 comctl32 msvcrt kernel32 ntdll)
add_pch(netshell precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET netshell DESTINATION reactos/system32 FOR all)
