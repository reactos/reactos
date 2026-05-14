
add_executable(route route.c route.rc)
set_module_type(route win32cui UNICODE)
target_compile_definitions(route PRIVATE _CRT_NON_CONFORMING_SWPRINTFS)
target_link_libraries(route conutils)
add_importlibs(route ws2_32 iphlpapi advapi32 msvcrt kernel32 ntdll) ## ntdll for RtlIpv*
add_cd_file(TARGET route DESTINATION reactos/system32 FOR all)
