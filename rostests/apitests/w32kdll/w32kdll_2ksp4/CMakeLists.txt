spec2def(w32kdll_2ksp4.dll w32kdll_2ksp4.spec)

add_library(w32kdll_2ksp4 SHARED 
    main.c 
    w32kdll_2ksp4.S
    ${CMAKE_CURRENT_BINARY_DIR}/w32kdll_2ksp4.def)
set_entrypoint(w32kdll_2ksp4 0)

add_dependencies(w32kdll_2ksp4 psdk)
