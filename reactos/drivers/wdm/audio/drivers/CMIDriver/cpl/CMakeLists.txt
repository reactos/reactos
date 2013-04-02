
add_library(cmicpl SHARED
    cmicpl.cpp
    cmicpl.rc
)

add_importlibs(cmicpl
    shell32
    msvcrt
    kernel32)

set_module_type(cmicpl cpl UNICODE)

add_cd_file(TARGET cmicpl DESTINATION reactos/system32/drivers FOR all)