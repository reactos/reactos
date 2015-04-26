
add_executable(pedump pedump.c pedump.rc)
set_module_type(pedump win32cui)
if(NOT MSVC)
    add_target_compile_flags(pedump "-Wno-unused-but-set-variable")
endif()
add_importlibs(pedump msvcrt kernel32 ntdll)
add_cd_file(TARGET pedump DESTINATION reactos/system32 FOR all)
