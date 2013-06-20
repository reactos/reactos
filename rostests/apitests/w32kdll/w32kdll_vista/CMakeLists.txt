
spec2def(w32kdll_vista.spec w32kdll_vista.spec ADD_IMPORTLIB)

add_library(w32kdll_vista SHARED
    main.c
    w32kdll_vista.S
    ${CMAKE_CURRENT_BINARY_DIR}/w32kdll_vista.def)
set_entrypoint(w32kdll_vista 0)

add_dependencies(w32kdll_vista psdk )
