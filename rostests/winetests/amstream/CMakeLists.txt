
add_definitions(-D_DLL -D__USE_CRTIMP -D__WINESRC__)
add_executable(amstream_winetest amstream.c testlist.c)
target_link_libraries(amstream_winetest wine uuid)
set_module_type(amstream_winetest win32cui)
add_importlibs(amstream_winetest ole32 user32 ddraw msvcrt kernel32 ntdll)
add_dependencies(amstream_winetest dxsdk)
add_cab_target(amstream_winetest 7)
