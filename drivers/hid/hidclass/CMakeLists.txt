
list(APPEND SOURCE
    fdo.c
    hidclass.c
    pdo.c
    precomp.h)

add_library(hidclass MODULE
    ${SOURCE}
    guid.c
    hidclass.rc)
spec2def(hidclass hidclass.sys hidclass.spec ADD_IMPORTLIB)

set_module_type(hidclass kernelmodedriver ENTRYPOINT 0)
add_importlibs(hidclass ntoskrnl hidparse hal)
add_pch(hidclass precomp.h SOURCE)
add_cd_file(TARGET hidclass DESTINATION reactos/system32/drivers NO_CAB FOR all)
