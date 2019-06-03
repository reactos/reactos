
add_asm_files(win32u_2k3sp2_asm sys-stubs.S)

add_library(win32u_2k3sp2 MODULE
    main.c
    ${win32u_2k3sp2_asm})
spec2def(win32u_2k3sp2 win32u_2k3sp2.dll win32u_2k3sp2.spec ADD_IMPORTLIB)

set_module_type(win32u_2k3sp2 module)
add_dependencies(win32u_2k3sp2 psdk)
