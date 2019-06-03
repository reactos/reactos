
add_definitions(-D_BATTERYCLASS_)

add_library(battc MODULE
            battc.c
            battc.rc)
spec2def(battc battc.sys battc.spec ADD_IMPORTLIB)

set_module_type(battc kernelmodedriver)
add_importlibs(battc ntoskrnl hal)
add_cd_file(TARGET battc DESTINATION reactos/system32/drivers FOR all)
