
add_definitions(
    -D__ROS_LONG64__
    -D_CRT_NONSTDC_NO_DEPRECATE
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    cpp.c
    data.c
    dir.c
    environ.c
    file.c
    headers.c
    heap.c
    locale.c
    misc.c
    printf.c
    scanf.c
    signal.c
    string.c
    testlist.c
    time.c)

add_executable(msvcrt_winetest ${SOURCE})
set_module_type(msvcrt_winetest win32cui)
add_importlibs(msvcrt_winetest msvcrt kernel32 ntdll)
