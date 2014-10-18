
spec2def(w32kdll_xpsp2.dll w32kdll_xpsp2.spec ADD_IMPORTLIB)

add_library(w32kdll_xpsp2 SHARED 
    main.c
    sys-stubs.S
    ${CMAKE_CURRENT_BINARY_DIR}/w32kdll_xpsp2.def)

set_module_type(w32kdll_xpsp2 module)
add_dependencies(w32kdll_xpsp2 psdk)
