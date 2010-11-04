
add_library(beep SHARED beep.c beep.rc)
set_module_type(beep kernelmodedriver)
add_importlibs(beep ntoskrnl hal)
add_cab_target(beep 2)
