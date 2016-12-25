
spec2def(win32kdll_vista.spec win32kdll_vista.spec ADD_IMPORTLIB)
add_asm_files(win32kdll_vista_asm win32kdll_vista.S)

add_library(win32kdll_vista SHARED
    main.c
    ${win32kdll_vista_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/win32kdll_vista.def)

set_module_type(win32kdll_vista module)
add_dependencies(win32kdll_vista psdk)
