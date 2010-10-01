
list(APPEND SOURCE
    cmbatt.c
    cmexec.c
    cmbpnp.c
    cmbwmi.c
    cmbatt.rc)

add_library(cmbatt SHARED ${CMAKE_CURRENT_BINARY_DIR}/cmbatt_cmbatt.h.gch ${SOURCE})

set_target_properties(cmbatt PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(cmbatt ntoskrnl hal battc wmilib)

add_pch(cmbatt ${CMAKE_CURRENT_SOURCE_DIR}/cmbatt.h ${SOURCE})
add_dependencies(cmbatt psdk bugcodes)
