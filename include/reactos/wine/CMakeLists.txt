
add_idl_headers(wineheaders itss.idl wined3d.idl)
add_idl_headers(winesdk epm.idl irot.idl)

add_rpc_library(epmrpc epm.idl)
add_rpc_library(irotrpc irot.idl)

add_iid_library(wined3d_guid wined3d.idl)
add_iid_library(itss_guid itss.idl)