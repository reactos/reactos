
include_directories(.)

list(APPEND SOURCE
    eventlogrpc.idl
    lsa.idl
    svcctl.idl
    wlansvc.idl)

idl_files(${SOURCE})

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

idl_files(pnp.idl)
