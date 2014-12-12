
spec2def(w32kdll_vista.spec w32kdll_vista.spec ADD_IMPORTLIB)
add_asm_files(w32kdll_vista_asm w32kdll_vista.S)

add_library(w32kdll_vista SHARED
    main.c
    ${w32kdll_vista_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/w32kdll_vista.def)

set_module_type(w32kdll_vista module)
add_dependencies(w32kdll_vista psdk)
