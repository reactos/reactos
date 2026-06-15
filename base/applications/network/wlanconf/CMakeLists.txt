
include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers/ndisuio)

add_executable(wlanconf wlanconf.c wlanconf.rc)
set_module_type(wlanconf win32cui UNICODE)
target_link_libraries(wlanconf conutils)
add_importlibs(wlanconf iphlpapi msvcrt kernel32)
add_cd_file(TARGET wlanconf DESTINATION reactos/system32 FOR all)
