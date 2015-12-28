
spec2def(win32kdll_2ksp4.dll win32kdll_2ksp4.spec)
add_asm_files(win32kdll_2ksp4_asm win32kdll_2ksp4.S)

add_library(win32kdll_2ksp4 SHARED 
    main.c 
    ${win32kdll_2ksp4_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/win32kdll_2ksp4.def)

set_module_type(win32kdll_2ksp4 module)
add_dependencies(win32kdll_2ksp4 psdk)
