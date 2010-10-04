
set_unicode()
include_directories(.)
set_rc_compiler()

add_executable(regsvr32 regsvr32.c regsvr32.rc)

set_module_type(regsvr32 win32gui)

add_importlibs(regsvr32 user32 shell32 ole32 msvcrt)
