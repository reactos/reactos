
spec2def(win32u_vista.dll win32u_vista.spec ADD_IMPORTLIB)
add_asm_files(win32u_vista_asm sys-stubs.S)

add_library(win32u_vista MODULE
    main.c
    ${win32u_vista_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/win32u_vista.def)

set_module_type(win32u_vista module)
add_dependencies(win32u_vista psdk)
add_rostests_file(TARGET win32u_vista)

if (STACK_PROTECTOR)
    target_compile_options(win32u_vista PRIVATE -fno-stack-protector)
endif()
