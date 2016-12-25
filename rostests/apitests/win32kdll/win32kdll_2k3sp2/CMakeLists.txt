
spec2def(win32kdll_2k3sp2.dll win32kdll_2k3sp2.spec ADD_IMPORTLIB)
add_asm_files(win32kdll_2k3sp2_asm sys-stubs.S)

add_library(win32kdll_2k3sp2 SHARED
    main.c
    ${win32kdll_2k3sp2_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/win32kdll_2k3sp2.def)

set_module_type(win32kdll_2k3sp2 module)
add_dependencies(win32kdll_2k3sp2 psdk)
