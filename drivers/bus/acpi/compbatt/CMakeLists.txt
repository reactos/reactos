
list(APPEND SOURCE
    compbatt.c
    compmisc.c
    comppnp.c
    compbatt.rc)

add_library(compbatt SHARED ${CMAKE_CURRENT_BINARY_DIR}/compbatt_compbatt.h.gch ${SOURCE})

set_module_type(compbatt kernelmodedriver)
add_importlibs(compbatt ntoskrnl hal battc)

add_pch(compbatt ${CMAKE_CURRENT_SOURCE_DIR}/compbatt.h ${SOURCE})

add_cd_file(TARGET compbatt DESTINATION reactos/system32/drivers FOR all)