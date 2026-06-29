
list(APPEND SOURCE
    cmbatt.c
    cmexec.c
    cmbpnp.c
    cmbwmi.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(cmbatt MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    cmbatt.rc)

set_module_type(cmbatt kernelmodedriver)
add_importlibs(cmbatt ntoskrnl hal battc wmilib)
add_pch(cmbatt cmbatt.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET cmbatt DESTINATION reactos/system32/drivers FOR all)
