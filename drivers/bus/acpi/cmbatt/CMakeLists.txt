
list(APPEND SOURCE
    cmbatt.c
    cmexec.c
    cmbpnp.c
    cmbwmi.c
    cmbatt.rc)

add_library(cmbatt SHARED ${SOURCE})

set_module_type(cmbatt kernelmodedriver)
add_importlibs(cmbatt ntoskrnl hal battc wmilib)

add_pch(cmbatt cmbatt.h)


add_cd_file(TARGET cmbatt DESTINATION reactos/system32/drivers FOR all)