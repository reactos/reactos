


list(APPEND SOURCE
    blockdev.c
    cleanup.c
    close.c
    create.c
    dir.c
    direntry.c
    dirwr.c
    ea.c
    fat.c
    fastio.c
    fcb.c
    finfo.c
    flush.c
    fsctl.c
    iface.c
    misc.c
    pnp.c
    rw.c
    shutdown.c
    string.c
    volume.c
    vfatfs.rc)

add_library(fastfat SHARED ${CMAKE_CURRENT_BINARY_DIR}/fastfat_vfat.h.gch ${SOURCE})

set_target_properties(fastfat PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(fastfat
    -lntoskrnl
    -lhal)

add_pch(fastfat ${CMAKE_CURRENT_SOURCE_DIR}/vfat.h ${SOURCE})
add_dependencies(fastfat psdk bugcodes)
add_minicd_target(fastfat reactos fastfat.sys)
add_livecd_target(fastfat reactos/system32/drivers)