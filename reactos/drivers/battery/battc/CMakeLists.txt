
add_definitions(-D_BATTERYCLASS_)

spec2def(batc.sys battc.spec)

add_library(battc SHARED
            battc.c
            battc.rc
            ${CMAKE_CURRENT_BINARY_DIR}/battc.def)

set_module_type(battc kernelmodedriver)
add_importlibs(battc ntoskrnl hal)

add_cd_file(TARGET battc DESTINATION reactos/system32/drivers FOR all)
add_importlib_target(battc.spec)
