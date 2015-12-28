
spec2def(win32kdll_xpsp2.dll win32kdll_xpsp2.spec ADD_IMPORTLIB)
add_asm_files(win32kdll_xpsp2_asm sys-stubs.S)

add_library(win32kdll_xpsp2 SHARED 
    main.c
    ${win32kdll_xpsp2_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/win32kdll_xpsp2.def)

set_module_type(win32kdll_xpsp2 module)
add_dependencies(win32kdll_xpsp2 psdk)
