
spec2def(osmesa.dll osmesa.spec)

add_library(osmesa SHARED
    osmesa.c
    ${CMAKE_CURRENT_BINARY_DIR}/osmesa.def)

target_link_libraries(osmesa
    mesa_drv_common
    mesa_glapi
    mesa_swrast
    mesa_swrast_setup
    mesa_tnl
    mesa_main
    mesa_vbo
    mesa_program
    mesa_math
    mesa_glsl
    ${mesa_asm_lib}
)

set_module_type(osmesa win32dll)
add_importlibs(osmesa msvcrt kernel32 ntdll)
add_cd_file(TARGET osmesa DESTINATION reactos/system32 FOR all) 