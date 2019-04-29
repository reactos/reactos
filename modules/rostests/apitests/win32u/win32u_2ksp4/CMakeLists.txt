
spec2def(win32u_2ksp4.dll win32u_2ksp4.spec)
add_asm_files(win32u_2ksp4_asm win32u_2ksp4.S)

add_library(win32u_2ksp4 MODULE 
    main.c 
    ${win32ku_2ksp4_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/win32u_2ksp4.def)

set_module_type(win32u_2ksp4 module)
add_dependencies(win32u_2ksp4 psdk)
