
spec2def(win32u_vista.spec win32u_vista.spec ADD_IMPORTLIB)
add_asm_files(win32u_vista_asm win32u_vista.S)

add_library(win32u_vista MODULE
    main.c
    ${win32u_vista_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/win32u_vista.def)

set_module_type(win32u_vista module)
add_dependencies(win32u_vista psdk)
