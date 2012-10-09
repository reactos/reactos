
add_definitions(-D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
     cmdline_util.c)

add_executable(cmdline_util ${SOURCE})
set_module_type(cmdline_util win32gui UNICODE)
add_importlibs(cmdline_util msvcrt kernel32 ntdll)
add_cd_file(TARGET cmdline_util DESTINATION reactos/bin FOR all)
