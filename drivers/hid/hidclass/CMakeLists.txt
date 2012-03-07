
spec2def(hidclass.sys hidclass.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    fdo.c
    hidclass.c
    hidclass.rc
    pdo.c
    ${CMAKE_CURRENT_BINARY_DIR}/hidclass.def)

add_library(hidclass SHARED ${SOURCE})

set_entrypoint(hidclass 0)
set_subsystem(hidclass native)
set_target_properties(hidclass PROPERTIES SUFFIX ".sys")


add_importlibs(hidclass ntoskrnl hidparse hal)


add_cd_file(TARGET hidclass DESTINATION reactos/system32/drivers NO_CAB FOR all)