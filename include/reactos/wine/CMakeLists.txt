
add_idl_headers(wineheaders itss.idl wined3d.idl)
add_idl_headers(winesdk epm.idl irot.idl)

add_rpc_library(epmrpc epm.idl)
add_rpc_library(irotrpc irot.idl)

