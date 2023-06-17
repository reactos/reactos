include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(ipconfig ipconfig.c ipconfig.rc)
set_module_type(ipconfig win32cui UNICODE)
target_link_libraries(ipconfig conutils ${PSEH_LIB})
add_importlibs(ipconfig user32 iphlpapi dnsapi advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ipconfig DESTINATION reactos/system32 FOR all)
