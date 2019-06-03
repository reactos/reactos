
list(APPEND SOURCE
    wmilib.c
    wmilib.rc)

add_library(wmilib MODULE ${SOURCE})
spec2def(wmilib wmilib.sys wmilib.spec ADD_IMPORTLIB)

set_module_type(wmilib kernelmodedriver ENTRYPOINT 0)
add_importlibs(wmilib ntoskrnl)
add_cd_file(TARGET wmilib DESTINATION reactos/system32/drivers NO_CAB FOR all)
