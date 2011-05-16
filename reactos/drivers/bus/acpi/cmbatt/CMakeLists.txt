
list(APPEND SOURCE
    cmbatt.c
    cmexec.c
    cmbpnp.c
    cmbwmi.c
    cmbatt.rc)

add_library(cmbatt SHARED ${CMAKE_CURRENT_BINARY_DIR}/cmbatt_cmbatt.h.gch ${SOURCE})

set_module_type(cmbatt kernelmodedriver)
add_importlibs(cmbatt ntoskrnl hal battc wmilib)

add_pch(cmbatt ${CMAKE_CURRENT_SOURCE_DIR}/cmbatt.h ${SOURCE})


add_cd_file(TARGET cmbatt DESTINATION reactos/system32/drivers FOR all)