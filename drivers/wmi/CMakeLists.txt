
spec2def(wmilib.sys wmilib.spec)

add_library(wmilib SHARED wmilib.c wmilib.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wmilib.def)

set_module_type(wmilib kernelmodedriver)

add_importlibs(wmilib ntoskrnl)
