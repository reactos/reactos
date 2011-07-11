
add_library(mup SHARED
    create.c
    mup.c
    mup.rc)

set_module_type(mup kernelmodedriver)
add_importlibs(mup ntoskrnl hal)

add_cd_file(TARGET mup DESTINATION reactos/system32/drivers FOR all)