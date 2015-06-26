
include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/idl
    ${CMAKE_CURRENT_BINARY_DIR})

add_rpc_files(server ${REACTOS_SOURCE_DIR}/include/reactos/idl/winreg.idl)

list(APPEND SOURCE
    environment.c
    rpcserver.c
    sas.c
    screensaver.c
    setup.c
    winlogon.c
    wlx.c
    winlogon.h
    ${CMAKE_CURRENT_BINARY_DIR}/winreg_s.c)

add_rc_deps(winlogon.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/winlogon.ico)
add_executable(winlogon ${SOURCE} winlogon.rc)
target_link_libraries(winlogon wine)
set_module_type(winlogon win32gui)
add_importlibs(winlogon user32 advapi32 userenv secur32 rpcrt4 msvcrt kernel32 ntdll)
add_pch(winlogon winlogon.h SOURCE)
add_cd_file(TARGET winlogon DESTINATION reactos/system32 FOR all)
