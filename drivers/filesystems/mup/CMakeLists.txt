
add_library(mup SHARED
    create.c
    mup.c
    mup.rc)

set_module_type(mup kernelmodedriver)
add_importlibs(mup ntoskrnl hal)
