
add_executable(CmdLineUtil CmdLineUtil.c)
set_module_type(CmdLineUtil win32gui UNICODE)
add_importlibs(CmdLineUtil msvcrt kernel32 ntdll)
add_rostests_file(TARGET CmdLineUtil SUBDIR testdata)
