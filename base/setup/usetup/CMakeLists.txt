
add_definitions(${I18N_DEFS})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${REACTOS_SOURCE_DIR}/sdk/lib/inflib
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

list(APPEND SOURCE
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
    cmdcons.c
    drivesup.c
    filesup.c
    filequeue.c
    format.c
    fslist.c
    genlist.c
    inffile.c
    inicache.c
    mui.c
    osdetect.c
    partlist.c
    progress.c
    registry.c
    settings.c
    usetup.h)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    #FIXME: Investigate Clang "illegal character encoding in string literal" warning
    set_property(SOURCE mui.c APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-invalid-source-encoding")
endif()

add_executable(usetup ${SOURCE} usetup.rc)
target_link_libraries(usetup zlib inflib ext2lib vfatlib)
set_module_type(usetup nativecui)
add_importlibs(usetup ntdll)
add_pch(usetup usetup.h SOURCE)
add_cd_file(TARGET usetup DESTINATION reactos/system32 NO_CAB NAME_ON_CD smss.exe FOR bootcd regtest)
