
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/udmihelp)

add_executable(systeminfo systeminfo.c systeminfo.rc)
set_module_type(systeminfo win32cui UNICODE)
target_link_libraries(systeminfo conutils udmihelp)
add_importlibs(systeminfo advapi32 netapi32 shlwapi iphlpapi ws2_32 msvcrt kernel32)
add_cd_file(TARGET systeminfo DESTINATION reactos/system32 FOR all)
