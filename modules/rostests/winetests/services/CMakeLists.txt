
if(GCC AND GCC_VERSION VERSION_GREATER 7)
    add_compile_flags("-Wno-format-overflow")
endif()

add_executable(services_winetest service.c testlist.c)
set_module_type(services_winetest win32cui)
add_importlibs(services_winetest user32 advapi32 msvcrt kernel32)
add_rostests_file(TARGET services_winetest)
