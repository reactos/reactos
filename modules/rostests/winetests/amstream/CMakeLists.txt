
add_definitions(-DCONST_VTABLE= -DWINETEST_WINE_DBG_STUBS)

add_executable(amstream_winetest amstream.c testlist.c)
target_link_libraries(amstream_winetest strmbase strmiids uuid dxguid)
set_module_type(amstream_winetest win32cui)
add_importlibs(amstream_winetest ole32 oleaut32 user32 gdi32 advapi32 ddraw kernel32_vista msvcrt kernel32 ntdll)
add_rostests_file(TARGET amstream_winetest)
add_dependencies(amstream_winetest dxsdk)
