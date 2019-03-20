
set_cpp()

add_library(cmicpl MODULE
    cmicpl.cpp
    cmicpl.rc)

set_module_type(cmicpl cpl UNICODE)
add_importlibs(cmicpl shell32 msvcrt kernel32)
add_cd_file(TARGET cmicpl DESTINATION reactos/system32/drivers FOR all)
