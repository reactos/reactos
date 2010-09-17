
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

list(APPEND SOURCE
    cdfs.c
    cleanup.c
    close.c
    common.c
    create.c
    dirctl.c
    fcb.c
    finfo.c
    fsctl.c
    misc.c
    rw.c
    volinfo.c
    cdfs.rc)

add_library(cdfs SHARED ${CMAKE_CURRENT_BINARY_DIR}/cdfs_cdfs.h.gch ${SOURCE})

set_target_properties(cdfs PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(cdfs
    -lntoskrnl
    -lhal)

add_pch(cdfs ${CMAKE_CURRENT_SOURCE_DIR}/cdfs.h ${SOURCE})
add_dependencies(cdfs psdk bugcodes)
