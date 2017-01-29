
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(ping ping.c ping.rc)
set_module_type(ping win32cui UNICODE)
target_link_libraries(ping conutils ${PSEH_LIB})
add_importlibs(ping ws2_32 iphlpapi msvcrt kernel32 ntdll)
add_cd_file(TARGET ping DESTINATION reactos/system32 FOR all)
