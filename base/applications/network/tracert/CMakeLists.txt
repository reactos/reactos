include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(tracert tracert.cpp tracert.rc)
set_module_type(tracert win32cui UNICODE)
target_link_libraries(tracert conutils ${PSEH_LIB})
add_importlibs(tracert ws2_32 iphlpapi msvcrt kernel32 ntdll)
add_cd_file(TARGET tracert DESTINATION reactos/system32 FOR all)
