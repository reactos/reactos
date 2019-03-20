
list(APPEND SOURCE
    compbatt.c
    compmisc.c
    comppnp.c
    compbatt.h)

add_library(compbatt MODULE
    ${SOURCE}
    guid.c
    compbatt.rc)

set_module_type(compbatt kernelmodedriver)
add_importlibs(compbatt ntoskrnl hal battc)
add_pch(compbatt compbatt.h SOURCE)
add_cd_file(TARGET compbatt DESTINATION reactos/system32/drivers FOR all)
