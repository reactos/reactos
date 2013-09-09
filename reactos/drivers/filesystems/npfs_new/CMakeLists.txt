
list(APPEND SOURCE
     create.c
     datasup.c
     fileobsup.c
     main.c
     prefxsup.c
     read.c
     readsup.c
     secursup.c
     statesup.c
     strucsup.c
     waitsup.c)

add_library(npfs_new SHARED ${SOURCE})
set_module_type(npfs_new kernelmodedriver)
target_link_libraries(npfs_new ${PSEH_LIB})
add_importlibs(npfs_new ntoskrnl hal)
add_pch(npfs_new npfs.h)
add_cd_file(TARGET npfs_new DESTINATION reactos/system32/drivers FOR all)
