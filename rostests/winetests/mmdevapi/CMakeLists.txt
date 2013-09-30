
add_executable(mmdevapi_winetest
    capture.c
    dependency.c
    mmdevenum.c
    propstore.c
    render.c
    testlist.c)

set_module_type(mmdevapi_winetest win32cui)
target_link_libraries(mmdevapi_winetest uuid)
add_importlibs(mmdevapi_winetest ole32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mmdevapi_winetest DESTINATION reactos/bin FOR all)
