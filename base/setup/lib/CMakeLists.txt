
add_definitions(${I18N_DEFS})
if(_WINKD_)
    add_definitions(-D_WINKD_)
endif()

include_directories(spapisup utils)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/pseh/include)

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

add_library(setuplib ${SOURCE})
add_pch(setuplib precomp.h SOURCE)
add_dependencies(setuplib xdk) # psdk
