
spec2def(w32kdll_xpsp2.dll w32kdll_xpsp2.spec ADD_IMPORTLIB)
add_asm_files(w32kdll_xpsp2_asm sys-stubs.S)

add_library(w32kdll_xpsp2 SHARED 
    main.c
    ${w32kdll_xpsp2_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/w32kdll_xpsp2.def)

set_module_type(w32kdll_xpsp2 module)
add_dependencies(w32kdll_xpsp2 psdk)
