
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/conutils
    ${REACTOS_SOURCE_DIR}/sdk/lib/udmihelp)

add_executable(systeminfo systeminfo.c systeminfo.rc)
set_module_type(systeminfo win32cui UNICODE)
target_link_libraries(systeminfo conutils udmihelp ${PSEH_LIB})
add_importlibs(systeminfo user32 advapi32 netapi32 shlwapi iphlpapi ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET systeminfo DESTINATION reactos/system32 FOR all)
