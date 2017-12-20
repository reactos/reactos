
add_definitions(
    -DWINETEST_USE_DBGSTR_LONGLONG
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

if(USE_CLANG_CL OR (NOT MSVC))
    add_target_compile_flags(msvcrt_winetest "-Wno-format")
endif()

set_module_type(msvcrt_winetest win32cui)
add_importlibs(msvcrt_winetest msvcrt kernel32)

if(MSVC)
    target_link_libraries(msvcrt_winetest oldnames)
    add_importlibs(msvcrt_winetest ntdll)
endif()

add_rostests_file(TARGET msvcrt_winetest)
