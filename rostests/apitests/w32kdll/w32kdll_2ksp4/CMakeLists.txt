
spec2def(w32kdll_2ksp4.dll w32kdll_2ksp4.spec)
add_asm_files(w32kdll_2ksp4_asm w32kdll_2ksp4.S)

add_library(w32kdll_2ksp4 SHARED 
    main.c 
    ${w32kdll_2ksp4_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/w32kdll_2ksp4.def)

set_module_type(w32kdll_2ksp4 module)
add_dependencies(w32kdll_2ksp4 psdk)
