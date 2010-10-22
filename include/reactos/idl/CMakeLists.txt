
include_directories(.)

list(APPEND SOURCE
    eventlogrpc.idl
    lsa.idl
    svcctl.idl
    wlansvc.idl)

MACRO_IDL_FILES(${SOURCE})
remove_definitions(-DWINNT=0x502)
add_definitions(-DWINNT=0x600)
MACRO_IDL_FILES(pnp.idl)
