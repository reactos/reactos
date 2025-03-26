list(APPEND SOURCE
    acchksup.c
    allocsup.c
    cachesup.c
    cleanup.c
    close.c
    create.c
    devctrl.c
    deviosup.c
    dirctrl.c
    dirsup.c
    dumpsup.c
    ea.c
    easup.c
    fatdata.c
    fatinit.c
    fatprocssrc.c
    fileinfo.c
    filobsup.c
    flush.c
    fsctrl.c
    fspdisp.c
    lockctrl.c
    namesup.c
    pnp.c
    read.c
    resrcsup.c
    shutdown.c
    splaysup.c
    strucsup.c
    timesup.c
    verfysup.c
    volinfo.c
    workque.c
    write.c
    fatprocs.h)

add_library(fastfat MODULE ${SOURCE} fastfat.rc)
set_module_type(fastfat kernelmodedriver)
target_link_libraries(fastfat ${PSEH_LIB} memcmp)
if(GDB AND NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(fastfat PRIVATE -O0)
endif()
add_importlibs(fastfat ntoskrnl hal)
add_pch(fastfat fatprocs.h SOURCE)
add_cd_file(TARGET fastfat DESTINATION reactos/system32/drivers NO_CAB FOR all)
