
spec2def(w32kdll_ros.dll w32kdll_ros.spec ADD_IMPORTLIB)

add_library(w32kdll_ros SHARED 
    main.c
    sys-stubs.S
    ${CMAKE_CURRENT_BINARY_DIR}/w32kdll_ros.def)

set_entrypoint(w32kdll_ros 0)

add_dependencies(w32kdll_ros psdk)
