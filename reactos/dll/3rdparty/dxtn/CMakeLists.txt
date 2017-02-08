
spec2def(dxtn.dll dxtn.spec)

add_library(dxtn SHARED
    fxt1.c
    dxtn.c
    wrapper.c
    texstore.c
    ${CMAKE_CURRENT_BINARY_DIR}/dxtn.def)

set_entrypoint(dxtn 0)
add_importlibs(dxtn msvcrt)
if(NOT MSVC)
    add_target_compile_flags(dxtn "-Wno-unused-but-set-variable")
endif()
add_cd_file(TARGET dxtn DESTINATION reactos/system32 FOR all)
