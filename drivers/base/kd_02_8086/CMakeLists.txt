spec2def(kd_02_8086.dll kd_02_8086.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    e8086.c
    kdext.c
)

add_library(kd_02_8086 MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/kd_02_8086.def)

set_module_type(kd_02_8086 kerneldll ENTRYPOINT 0)
set_subsystem(kd_02_8086 native)

add_importlibs(kd_02_8086 ntoskrnl hal)

add_dependencies(kd_02_8086 psdk bugcodes)
add_cd_file(TARGET kd_02_8086 DESTINATION reactos/system32 NO_CAB FOR all)
