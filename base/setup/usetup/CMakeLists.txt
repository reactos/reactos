
add_definitions(${I18N_DEFS})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
#    ${REACTOS_SOURCE_DIR}/base/setup/lib
    ${REACTOS_SOURCE_DIR}/sdk/lib/inflib
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

list(APPEND SOURCE
    spapisup/cabinet.c
    spapisup/fileqsup.c
    spapisup/infsupp.c
    cmdcons.c
    console.c
    consup.c
    devinst.c
    fmtchk.c
    fslist.c
    genlist.c
    keytrans.c
    mui.c
    partlist.c
    progress.c
    usetup.c
    usetup.h)

add_executable(usetup ${SOURCE} usetup.rc)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(usetup PRIVATE "-Wno-invalid-source-encoding")
endif()

add_pch(usetup usetup.h SOURCE)
set_module_type(usetup nativecui)
target_link_libraries(usetup inflib setuplib zlib_solo ext2lib vfatlib btrfslib chkstk ${PSEH_LIB})
add_importlibs(usetup ntdll)
add_cd_file(TARGET usetup DESTINATION reactos/system32 NO_CAB NAME_ON_CD smss.exe FOR bootcd regtest)
