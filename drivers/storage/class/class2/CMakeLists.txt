
include_directories(..)

list(APPEND SOURCE
    class2.c
    class2.rc)

add_library(class2 MODULE ${SOURCE})
spec2def(class2 class2.sys class2.spec ADD_IMPORTLIB)

set_module_type(class2 kernelmodedriver)
add_importlibs(class2 scsiport ntoskrnl hal)
add_cd_file(TARGET class2 DESTINATION reactos/system32/drivers NO_CAB FOR all)
