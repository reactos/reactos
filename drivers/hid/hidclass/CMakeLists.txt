
spec2def(hidclass.sys hidclass.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    fdo.c
    hidclass.c
    pdo.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(hidclass MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    hidclass.rc
    ${CMAKE_CURRENT_BINARY_DIR}/hidclass.def)

set_module_type(hidclass kernelmodedriver ENTRYPOINT 0)
add_importlibs(hidclass ntoskrnl hidparse hal)
add_pch(hidclass precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET hidclass DESTINATION reactos/system32/drivers NO_CAB FOR all)
