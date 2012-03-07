
spec2def(w32kdll_2k3sp2.dll w32kdll_2k3sp2.spec ADD_IMPORTLIB)

add_library(w32kdll_2k3sp2 SHARED
    main.c
    w32kdll_2k3sp2.S
    ${CMAKE_CURRENT_BINARY_DIR}/w32kdll_2k3sp2.def)

set_entrypoint(w32kdll_2k3sp2 0)

add_dependencies(w32kdll_2k3sp2 psdk )
