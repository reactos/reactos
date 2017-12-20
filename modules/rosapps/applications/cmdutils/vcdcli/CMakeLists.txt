
include_directories(${REACTOS_SOURCE_DIR}/modules/rosapps/drivers/vcdrom)
add_executable(vcdcli vcdcli.c vcdcli.rc)
set_module_type(vcdcli win32cui UNICODE)
add_importlibs(vcdcli advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET vcdcli DESTINATION reactos/system32 FOR all)
