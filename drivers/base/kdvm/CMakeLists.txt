
spec2def(kdvbox.dll kdvm.spec)
add_asm_files(kdvbox_asm kdvbox_asm.S)

add_library(kdvbox SHARED
    kdvm.c
    kdvbox.c
    kdvm.rc
    ${kdvbox_asm}
    ${CMAKE_CURRENT_BINARY_DIR}/kdvbox.def)

set_module_type(kdvbox module IMAGEBASE 0x00010000)
set_subsystem(kdvbox native)
add_importlibs(kdvbox ntoskrnl hal)
add_dependencies(kdvbox psdk bugcodes)
add_cd_file(TARGET kdvbox DESTINATION reactos/system32 NO_CAB FOR all)
