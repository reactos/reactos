
include_directories(${REACTOS_SOURCE_DIR}/win32ss)

spec2def(win32kdll.dll win32kdll_ros.spec ADD_IMPORTLIB)
add_asm_files(win32kdll_ros_asm sys-stubs.S)

add_library(win32kdll SHARED 
    main.c
    ${win32kdll_ros_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/win32kdll.def)

set_module_type(win32kdll module)
add_dependencies(win32kdll psdk)
add_cd_file(TARGET win32kdll DESTINATION reactos/bin FOR all)
