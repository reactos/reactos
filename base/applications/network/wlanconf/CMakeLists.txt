
include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers/ndisuio)
add_executable(wlanconf wlanconf.c wlanconf.rc)
set_module_type(wlanconf win32cui)
add_importlibs(wlanconf msvcrt iphlpapi kernel32)
add_cd_file(TARGET wlanconf DESTINATION reactos/system32 FOR all)
