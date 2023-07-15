
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)
spec2def(netsh.exe netsh.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    context.c
    help.c
    helper.c
    interpreter.c
    netsh.c
    precomp.h)

add_executable(netsh ${SOURCE} netsh.rc ${CMAKE_CURRENT_BINARY_DIR}/netsh.def)

set_target_properties(netsh
    PROPERTIES
    ENABLE_EXPORTS TRUE
    DEFINE_SYMBOL "")

set_module_type(netsh win32cui UNICODE)
target_link_libraries(netsh conutils ${PSEH_LIB})
add_importlibs(netsh advapi32 msvcrt user32 kernel32 ntdll)

add_pch(netsh precomp.h SOURCE)
add_cd_file(TARGET netsh DESTINATION reactos/system32 FOR all)
