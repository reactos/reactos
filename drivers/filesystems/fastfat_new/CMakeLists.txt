
include_directories(.)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/fullfat)

list(APPEND SOURCE
    cleanup.c
    close.c
    create.c
    device.c
    dir.c
    ea.c
    fastfat.c
    fastio.c
    fat.c
    fcb.c
    finfo.c
    flush.c
    fsctl.c
    fullfat.c
    lock.c
    rw.c
    shutdown.c
    volume.c
    fastfat.rc)

add_library(fastfat_new SHARED ${CMAKE_CURRENT_BINARY_DIR}/fastfat_new_fastfat.h.gch ${SOURCE})

set_target_properties(fastfat_new PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(fastfat_new
    pseh
    fullfat)

add_importlibs(fastfat_new ntoskrnl hal)

add_pch(fastfat_new ${CMAKE_CURRENT_SOURCE_DIR}/fastfat.h ${SOURCE})
add_dependencies(fastfat_new psdk bugcodes)
