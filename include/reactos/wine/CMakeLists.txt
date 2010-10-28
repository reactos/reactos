
ADD_INTERFACE_DEFINITIONS(wineheaders itss.idl wined3d.idl)

list(APPEND SOURCE
    epm.idl
    irot.idl)

MACRO_IDL_FILES(${SOURCE})
ADD_INTERFACE_DEFINITIONS(winesdk ${SOURCE})