spec2def(w32kdll_xpsp2.dll w32kdll_xpsp2.spec ADD_IMPORTLIB)

add_library(w32kdll_xpsp2 SHARED 
    main.c
    w32kdll_xpsp2.S
    ${CMAKE_CURRENT_BINARY_DIR}/w32kdll_xpsp2.def)
set_entrypoint(w32kdll_xpsp2 0)

add_dependencies(w32kdll_xpsp2 psdk)
