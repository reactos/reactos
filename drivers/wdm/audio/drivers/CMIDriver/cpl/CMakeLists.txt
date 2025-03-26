

add_library(cmicpl MODULE
    cmicpl.cpp
    cmicpl.rc)

add_dependencies(cmicpl psdk)
set_module_type(cmicpl cpl UNICODE)
add_importlibs(cmicpl shell32 msvcrt kernel32)
add_cd_file(TARGET cmicpl DESTINATION reactos/system32/drivers FOR all)
