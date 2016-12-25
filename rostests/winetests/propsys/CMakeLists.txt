
add_definitions(-DUSE_WINE_TODOS)
add_executable(propsys_winetest propstore.c propsys.c testlist.c)
set_module_type(propsys_winetest win32cui)
add_importlibs(propsys_winetest propsys ole32 oleaut32 msvcrt kernel32)

if(MSVC)
    add_importlibs(propsys_winetest ntdll)
endif()

add_cd_file(TARGET propsys_winetest DESTINATION reactos/bin FOR all)
