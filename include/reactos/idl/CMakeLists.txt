
include_directories(.)

add_rpc_library(eventlogrpc eventlogrpc.idl)
add_rpc_library(lsarpc lsa.idl)
add_rpc_library(svcctlrpc svcctl.idl)
add_rpc_library(wlansvcrpc wlansvc.idl)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_rpc_library(pnprpc pnp.idl)
