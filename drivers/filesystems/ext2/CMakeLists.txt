include_directories(inc)

list(APPEND SOURCE
    src/volinfo.c
    src/create.c
    src/write.c
    src/metadata.c
    src/fastio.c
    src/dircntrl.c
    src/DiskIO.c
    src/fsctrl.c
    src/devcntrl.c
    src/flush.c
    src/ext2init.c
    src/io.c
    src/close.c
    src/fileinfo.c
    src/read.c
    src/cleanup.c
    src/misc.c
    src/shutdown.c)
    
add_library(ext2fs SHARED ${SOURCE})

set_target_properties(ext2fs PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")


target_link_libraries(ext2fs
    -lntoskrnl
    -lhal)

add_dependencies(ext2fs psdk bugcodes)
