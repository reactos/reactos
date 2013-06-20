
add_definitions(
    -D__ROS_LONG64__
    -D_CRT_NONSTDC_NO_DEPRECATE
    -D__msvcrt_ulong=ULONG)

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

if(MSVC)
    target_link_libraries(msvcrt_winetest oldnames)
else()
    add_target_compile_flags(msvcrt_winetest "-Wno-format")
endif()

set_module_type(msvcrt_winetest win32cui)
add_importlibs(msvcrt_winetest msvcrt kernel32 ntdll)
add_cd_file(TARGET msvcrt_winetest DESTINATION reactos/bin FOR all)
