
include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers/ndisuio)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(wlanconf wlanconf.c wlanconf.rc)
set_module_type(wlanconf win32cui UNICODE)
target_link_libraries(wlanconf conutils ${PSEH_LIB})
add_importlibs(wlanconf msvcrt iphlpapi kernel32)
add_cd_file(TARGET wlanconf DESTINATION reactos/system32 FOR all)
