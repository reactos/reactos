
add_definitions(-D__ROS_LONG64__)
add_executable(msctf_winetest inputprocessor.c testlist.c)
target_link_libraries(msctf_winetest wine)
set_module_type(msctf_winetest win32cui)
add_importlibs(msctf_winetest ole32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msctf_winetest DESTINATION reactos/bin FOR all)
