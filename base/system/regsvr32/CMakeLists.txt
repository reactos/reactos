
add_definitions(-D_DLL -D__USE_CRTIMP)

set_unicode()
include_directories(.)
set_rc_compiler()

add_executable(regsvr32 regsvr32.c regsvr32.rc)

set_module_type(regsvr32 win32gui)
add_importlibs(regsvr32 user32 shell32 ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET regsvr32 DESTINATION reactos/system32 FOR all)
