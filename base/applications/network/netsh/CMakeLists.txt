
spec2def(netsh.exe netsh.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    alias.c
    context.c
    help.c
    helper.c
    interpreter.c
    netsh.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/netsh_stubs.c)

add_executable(netsh ${SOURCE} netsh.rc ${CMAKE_CURRENT_BINARY_DIR}/netsh.def)
add_pch(netsh precomp.h SOURCE)

set_target_properties(netsh
    PROPERTIES
    ENABLE_EXPORTS TRUE
    DEFINE_SYMBOL "")

set_module_type(netsh win32cui UNICODE)
target_link_libraries(netsh conutils)
add_importlibs(netsh iphlpapi advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET netsh DESTINATION reactos/system32 FOR all)
