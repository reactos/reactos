
spec2def(wmilib.sys wmilib.spec)

list(APPEND SOURCE
    wmilib.c
    wmilib.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wmilib.def)

add_library(wmilib SHARED ${SOURCE})

set_module_type(wmilib kernelmodedriver ENTRYPOINT 0)

add_importlibs(wmilib ntoskrnl)
add_importlib_target(wmilib.spec)
add_cd_file(TARGET wmilib DESTINATION reactos/system32/drivers FOR all)
