
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(route route.c route.rc)
set_module_type(route win32cui UNICODE)
target_link_libraries(route conutils ${PSEH_LIB})
add_importlibs(route ws2_32 iphlpapi msvcrt advapi32 kernel32 ntdll)
add_cd_file(TARGET route DESTINATION reactos/system32 FOR all)
