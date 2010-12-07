
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP
    -D_UNICODE -DUNICODE)

add_executable(powrprof_winetest pwrprof.c testlist.c)
set_module_type(powrprof_winetest win32cui)
add_importlibs(powrprof_winetest advapi32 pwrprof msvcrt kernel32 ntdll)
