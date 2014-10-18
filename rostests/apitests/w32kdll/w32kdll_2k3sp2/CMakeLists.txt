
spec2def(w32kdll_2k3sp2.dll w32kdll_2k3sp2.spec ADD_IMPORTLIB)

add_library(w32kdll_2k3sp2 SHARED
    main.c
    sys-stubs.S
    ${CMAKE_CURRENT_BINARY_DIR}/w32kdll_2k3sp2.def)

set_module_type(w32kdll_2k3sp2 module)
add_dependencies(w32kdll_2k3sp2 psdk)
