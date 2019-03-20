
list(APPEND SOURCE
    cmbatt.c
    cmexec.c
    cmbpnp.c
    cmbwmi.c
    cmbatt.h)

add_library(cmbatt MODULE
    ${SOURCE}
    guid.c
    cmbatt.rc)

set_module_type(cmbatt kernelmodedriver)
add_importlibs(cmbatt ntoskrnl hal battc wmilib)
add_pch(cmbatt cmbatt.h SOURCE)
add_cd_file(TARGET cmbatt DESTINATION reactos/system32/drivers FOR all)
