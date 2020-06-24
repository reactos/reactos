
add_definitions(
    -DWINETEST_USE_DBGSTR_LONGLONG
    -D_CRT_NONSTDC_NO_DEPRECATE
    -D__msvcrt_ulong=ULONG)

if(GCC)
    add_compile_flags("-Wno-stringop-truncation")
endif()

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
    time.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(msvcrt_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

if(USE_CLANG_CL OR (NOT MSVC))
    target_compile_options(msvcrt_winetest PRIVATE "-Wno-format")
endif()

set_module_type(msvcrt_winetest win32cui)
add_importlibs(msvcrt_winetest advapi32 msvcrt kernel32)

if(MSVC)
    target_link_libraries(msvcrt_winetest oldnames)
    add_importlibs(msvcrt_winetest ntdll)
endif()

add_pch(msvcrt_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET msvcrt_winetest)
