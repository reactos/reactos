
list(APPEND SOURCE
    compbatt.c
    compmisc.c
    comppnp.c
    compbatt.rc)

add_library(compbatt SHARED ${SOURCE})

set_module_type(compbatt kernelmodedriver)
add_importlibs(compbatt ntoskrnl hal battc)

add_pch(compbatt compbatt.h)

add_cd_file(TARGET compbatt DESTINATION reactos/system32/drivers FOR all)
