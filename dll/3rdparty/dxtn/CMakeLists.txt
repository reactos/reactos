
spec2def(dxtn.dll dxtn.spec)

add_library(dxtn MODULE
    txc_compress_dxtn.c
    txc_fetch_dxtn.c
    ${CMAKE_CURRENT_BINARY_DIR}/dxtn.def)

set_module_type(dxtn win32dll ENTRYPOINT 0)
add_importlibs(dxtn msvcrt kernel32)
add_cd_file(TARGET dxtn DESTINATION reactos/system32 FOR all)
