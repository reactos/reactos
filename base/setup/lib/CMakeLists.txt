
add_definitions(${I18N_DEFS})
if(_WINKD_)
    add_definitions(-D_WINKD_)
endif()
add_definitions(-D_SETUPLIB_)

include_directories(spapisup utils)

spec2def(setuplib.dll setuplib.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    spapisup/fileqsup.c
    spapisup/infsupp.c
    utils/arcname.c
    utils/bldrsup.c
    utils/devutils.c
    utils/filesup.c
    utils/fsrec.c
    utils/genlist.c
    utils/inicache.c
    utils/ntverrsrc.c
    utils/osdetect.c
    utils/partinfo.c
    utils/partlist.c
    utils/regutil.c
    utils/volutil.c
    bootcode.c
    bootsup.c
    fsutil.c
    install.c
    mui.c
    registry.c
    settings.c
    setuplib.c
    precomp.h)

add_library(setuplib SHARED
    ${SOURCE}
    setuplib.rc
    ${CMAKE_CURRENT_BINARY_DIR}/setuplib.def)

add_pch(setuplib precomp.h SOURCE)
add_dependencies(setuplib xdk) # psdk

set_module_type(setuplib nativedll)
target_link_libraries(setuplib ext2lib vfatlib btrfslib ${PSEH_LIB})
add_importlibs(setuplib ntdll)
add_cd_file(TARGET setuplib DESTINATION reactos/system32 NO_CAB FOR bootcd regtest)
