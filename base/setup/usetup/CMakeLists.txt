
include_directories(${REACTOS_SOURCE_DIR}/lib/newinflib)
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/zlib)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

add_executable(usetup WIN32
    interface/usetup.c
    interface/devinst.c
    interface/consup.c
    native/utils/keytrans.c
    native/utils/console.c
    native/fslist.c
    native/console.c
    bootsup.c
    cabinet.c
    chkdsk.c
    drivesup.c
    filesup.c
    filequeue.c
    format.c
    fslist.c
    genlist.c
    inffile.c
    inicache.c
    mui.c
    partlist.c
    progress.c
    registry.c
    settings.c
    usetup.rc)

target_link_libraries(usetup
    mingw_main
    zlib
    newinflib
    ext2lib
    vfatlib
    mingw_common)

set_module_type(usetup nativecui)

add_dependencies(usetup psdk buildno_header)
