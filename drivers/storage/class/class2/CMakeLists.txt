
include_directories(..)
spec2def(class2.sys class2.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    class2.c
    class2.rc
    ${CMAKE_CURRENT_BINARY_DIR}/class2.def)

add_library(class2 MODULE ${SOURCE})
set_module_type(class2 kernelmodedriver)
add_importlibs(class2 scsiport ntoskrnl hal)
add_cd_file(TARGET class2 DESTINATION reactos/system32/drivers NO_CAB FOR all)
