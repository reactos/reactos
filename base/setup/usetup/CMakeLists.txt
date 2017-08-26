
add_definitions(${I18N_DEFS})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
#    ${REACTOS_SOURCE_DIR}/base/setup/lib
    ${REACTOS_SOURCE_DIR}/sdk/lib/inflib
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

list(APPEND SOURCE
    native/console.c
    native/keytrans.c
    native/host_native.c
    cabinet.c
    chkdsk.c
    cmdcons.c
    consup.c
    devinst.c
    filesup.c
    filequeue.c
    format.c
    fslist.c
    genlist.c
    inffile.c
    mui.c
    partlist.c
    progress.c
    settings.c
    usetup.c
    usetup.h)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    #FIXME: Investigate Clang "illegal character encoding in string literal" warning
    set_property(SOURCE mui.c APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-invalid-source-encoding")
endif()

add_executable(usetup ${SOURCE} usetup.rc)
add_pch(usetup usetup.h SOURCE)
set_module_type(usetup nativecui)
target_link_libraries(usetup inflib setuplib zlib ext2lib vfatlib)
add_importlibs(usetup ntdll)
add_cd_file(TARGET usetup DESTINATION reactos/system32 NO_CAB NAME_ON_CD smss.exe FOR bootcd regtest)
