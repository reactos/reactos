
add_definitions(-DWINETEST_USE_DBGSTR_LONGLONG)

add_executable(mmdevapi_winetest
    capture.c
    dependency.c
    mmdevenum.c
    propstore.c
    render.c
    testlist.c)

set_module_type(mmdevapi_winetest win32cui TARGET_VERSION vista)
target_link_libraries(mmdevapi_winetest uuid)
add_importlibs(mmdevapi_winetest ole32 user32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(mmdevapi_winetest dxsdk)
add_rostests_file(TARGET mmdevapi_winetest)
