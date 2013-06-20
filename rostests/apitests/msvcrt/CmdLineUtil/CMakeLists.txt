
list(APPEND SOURCE
     CmdLineUtil.c)

add_executable(CmdLineUtil ${SOURCE})
set_module_type(CmdLineUtil win32gui UNICODE)
add_importlibs(CmdLineUtil msvcrt kernel32 ntdll)
add_cd_file(TARGET CmdLineUtil DESTINATION reactos/bin/data FOR all)
