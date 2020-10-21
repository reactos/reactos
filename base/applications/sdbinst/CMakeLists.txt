include_directories(${REACTOS_SOURCE_DIR}/dll/appcompat/apphelp)
add_executable(sdbinst sdbinst.c)

set_module_type(sdbinst win32cui UNICODE)
add_importlibs(sdbinst msvcrt kernel32 ntdll advapi32 apphelp ole32)
add_cd_file(TARGET sdbinst DESTINATION reactos/system32 FOR all)
