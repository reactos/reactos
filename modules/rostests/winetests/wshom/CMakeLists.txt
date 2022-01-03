add_definitions(-DUSE_WINE_TODOS)

add_idl_headers(wshom_winetest_idlheaders wshom.idl)
add_executable(wshom_winetest wshom.c testlist.c)
set_module_type(wshom_winetest win32cui TARGET_VERSION vista)
add_importlibs(wshom_winetest oleaut32 ole32 advapi32 msvcrt kernel32)
add_dependencies(wshom_winetest stdole2 wshom_winetest_idlheaders)
add_rostests_file(TARGET wshom_winetest)
