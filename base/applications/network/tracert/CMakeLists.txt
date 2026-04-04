
add_executable(tracert tracert.cpp tracert.rc)
set_module_type(tracert win32cui UNICODE)
target_link_libraries(tracert conutils)
add_importlibs(tracert ws2_32 iphlpapi msvcrt kernel32)
add_cd_file(TARGET tracert DESTINATION reactos/system32 FOR all)
