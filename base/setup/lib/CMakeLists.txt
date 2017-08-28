
include_directories(utils)

list(APPEND SOURCE
    utils/arcname.c
    utils/bldrsup.c
    utils/filesup.c
    utils/genlist.c
    utils/infsupp.c
    utils/inicache.c
    utils/ntverrsrc.c
    utils/osdetect.c
    utils/partlist.c
    utils/regutil.c
    bootsup.c
    fsutil.c
    registry.c
    settings.c
    setuplib.c
    precomp.h)

add_library(setuplib ${SOURCE})
add_pch(setuplib precomp.h SOURCE)
add_dependencies(setuplib xdk) # psdk
