
spec2def(hidclass.sys hidclass.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    fdo.c
    hidclass.c
    pdo.c
    precomp.h)

add_library(hidclass SHARED
    ${SOURCE}
    guid.c
    hidclass.rc
    ${CMAKE_CURRENT_BINARY_DIR}/hidclass.def)

set_module_type(hidclass kernelmodedriver ENTRYPOINT 0)
add_importlibs(hidclass ntoskrnl hidparse hal)
add_pch(hidclass precomp.h SOURCE)
add_cd_file(TARGET hidclass DESTINATION reactos/system32/drivers NO_CAB FOR all)
