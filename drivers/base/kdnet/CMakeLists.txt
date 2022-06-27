spec2def(kdnet.dll kdnet.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    kdnet.c
    )

add_library(kdnet MODULE
    ${SOURCE}
    kdnet.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kdnet.def)

set_module_type(kdnet kerneldll ENTRYPOINT 0)
set_subsystem(kdnet native)
add_importlibs(kdnet ntoskrnl hal kdstub)
add_dependencies(kdnet psdk bugcodes)
add_cd_file(TARGET kdnet DESTINATION reactos/system32 NO_CAB FOR all)
