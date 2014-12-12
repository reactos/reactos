
spec2def(w32kdll_ros.dll w32kdll_ros.spec ADD_IMPORTLIB)
add_asm_files(w32kdll_ros_asm sys-stubs.S)

add_library(w32kdll_ros SHARED 
    main.c
    ${w32kdll_ros_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/w32kdll_ros.def)

set_module_type(w32kdll_ros module)
add_dependencies(w32kdll_ros psdk)
