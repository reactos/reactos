
include_directories(..)

spec2def(class2.sys class2.spec)

add_library(class2 SHARED
    class2.c
    class2.rc
    ${CMAKE_CURRENT_BINARY_DIR}/class2.def)

set_module_type(class2 kernelmodedriver)
add_importlibs(class2 scsiport ntoskrnl hal)

add_minicd_target(class2 reactos class2.sys)
add_livecd_target(class2 reactos/system32/drivers)
