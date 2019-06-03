
include_directories(${REACTOS_SOURCE_DIR}/win32ss)

add_asm_files(win32u_ros_asm sys-stubs.S)

add_library(win32u MODULE 
    main.c
    ${win32u_ros_asm})
spec2def(win32u win32u.dll win32u_ros.spec ADD_IMPORTLIB)

set_module_type(win32u module)
add_dependencies(win32u psdk)
add_rostests_file(TARGET win32u)
