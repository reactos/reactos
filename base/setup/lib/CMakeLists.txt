
add_definitions(${I18N_DEFS})

include_directories(spapisup utils)

list(APPEND SOURCE
    spapisup/fileqsup.c
    spapisup/infsupp.c
    utils/arcname.c
    utils/bldrsup.c
    utils/filesup.c
    utils/genlist.c
    utils/inicache.c
    utils/ntverrsrc.c
    utils/osdetect.c
    utils/partlist.c
    utils/regutil.c
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
