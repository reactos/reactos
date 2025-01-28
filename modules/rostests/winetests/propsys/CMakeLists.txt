
add_definitions(-DUSE_WINE_TODOS -DWINETEST_USE_DBGSTR_LONGLONG)
remove_definitions(-D__ROS_LONG64__)
add_executable(propsys_winetest propsys.c testlist.c)
set_module_type(propsys_winetest win32cui)
add_importlibs(propsys_winetest propsys ole32 oleaut32 msvcrt kernel32)

if(MSVC)
    add_importlibs(propsys_winetest ntdll)
endif()

add_rostests_file(TARGET propsys_winetest)
