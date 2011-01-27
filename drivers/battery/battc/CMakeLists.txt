
add_definitions(-D_BATTERYCLASS_)

spec2def(batc.sys battc.spec)

add_library(battc SHARED
            battc.c
            battc.rc
            ${CMAKE_CURRENT_BINARY_DIR}/battc.def)

set_module_type(battc kernelmodedriver)
add_importlibs(battc ntoskrnl hal)

add_cab_target(battc 2)
add_importlib_target(battc.spec)
