
add_interface_definitions(wineheaders itss.idl wined3d.idl)

list(APPEND SOURCE
    epm.idl
    irot.idl)

idl_files(${SOURCE})
add_interface_definitions(winesdk ${SOURCE})
