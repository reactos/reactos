
include_directories(
    ${REACTOS_SOURCE_DIR}/lib/newinflib
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/zlib
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers)

add_executable(usetup
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

if(MSVC)
    target_link_libraries(usetup msvcsup)
endif()

set_module_type(usetup nativecui)
add_importlibs(usetup kernel32 ntdll)
add_minicd_target(usetup reactos/system32 smss.exe)
