
add_definitions(-D_BATTERYCLASS_)

add_library(battc SHARED battc.c battc.rc)

set_module_type(battc kernelmodedriver)
add_importlibs(battc ntoskrnl hal)
