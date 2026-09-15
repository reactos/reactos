
add_executable(jsproxy_winetest jsproxy.c)
set_module_type(jsproxy_winetest win32cui)
add_importlibs(jsproxy_winetest ole32 msvcrt kernel32)
target_link_libraries(jsproxy_winetest winetestlib)
add_rostests_file(TARGET jsproxy_winetest)
