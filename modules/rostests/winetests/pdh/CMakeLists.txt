
add_definitions(-DWINETEST_USE_DBGSTR_LONGLONG)
add_executable(pdh_winetest pdh.c testlist.c)
set_module_type(pdh_winetest win32cui)
add_importlibs(pdh_winetest pdh msvcrt kernel32)

if(MSVC)
    add_importlibs(pdh_winetest ntdll)
endif()

add_rostests_file(TARGET pdh_winetest)
