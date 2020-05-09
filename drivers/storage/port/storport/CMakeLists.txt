
spec2def(storport.sys storport.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    fdo.c
    miniport.c
    misc.c
    pdo.c
    storport.c
    stubs.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(storport MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    storport.rc
    ${CMAKE_CURRENT_BINARY_DIR}/storport.def)

add_pch(storport precomp.h "${PCH_SKIP_SOURCE}")
set_module_type(storport kernelmodedriver)
add_importlibs(storport ntoskrnl hal)
add_cd_file(TARGET storport DESTINATION reactos/system32/drivers NO_CAB FOR all)
