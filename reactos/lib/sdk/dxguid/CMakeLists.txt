
add_library(dxguid dxguid-mingw.c)
add_library(dx10guid dx10guid.c)

add_dependencies(dxguid psdk)
add_dependencies(dx10guid psdk d3d_idl_headers)
