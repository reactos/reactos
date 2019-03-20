
add_definitions(-D_BATTERYCLASS_)
spec2def(battc.sys battc.spec ADD_IMPORTLIB)

add_library(battc MODULE
            battc.c
            battc.rc
            ${CMAKE_CURRENT_BINARY_DIR}/battc.def)

set_module_type(battc kernelmodedriver)
add_importlibs(battc ntoskrnl hal)
add_cd_file(TARGET battc DESTINATION reactos/system32/drivers FOR all)
