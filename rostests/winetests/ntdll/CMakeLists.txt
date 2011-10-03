
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    atom.c
    change.c
    directory.c
    env.c
    error.c
    exception.c
    file.c
    generated.c
    info.c
    large_int.c
    om.c
    path.c
    pipe.c
    port.c
    reg.c
    rtl.c
    rtlbitmap.c
    rtlstr.c
    string.c
    time.c
    testlist.c)

add_executable(ntdll_winetest ${SOURCE})
set_module_type(ntdll_winetest win32cui)
add_importlibs(ntdll_winetest user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ntdll_winetest DESTINATION reactos/bin FOR all)
if(NOT MSVC)
    allow_warnings(ntdll_winetest)
endif()
