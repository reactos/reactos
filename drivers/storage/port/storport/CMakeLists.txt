
spec2def(storport.sys storport.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    fdo.c
    miniport.c
    misc.c
    pdo.c
    storport.c
    stubs.c
    precomp.h)

add_library(storport MODULE
    ${SOURCE}
    guid.c
    storport.rc
    ${CMAKE_CURRENT_BINARY_DIR}/storport.def)

add_pch(storport precomp.h SOURCE)
set_module_type(storport kernelmodedriver)
add_importlibs(storport ntoskrnl hal)
add_cd_file(TARGET storport DESTINATION reactos/system32/drivers NO_CAB FOR all)
