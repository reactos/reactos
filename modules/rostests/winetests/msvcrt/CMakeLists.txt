
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

target_compile_definitions(msvcrt_winetest PRIVATE
    WINETEST_USE_DBGSTR_LONGLONG
    _CRT_NONSTDC_NO_DEPRECATE
    __msvcrt_ulong=ULONG)

if ((NOT MSVC) OR USE_CLANG_CL)
    if (NOT USE_CLANG_CL)
        target_compile_options(msvcrt_winetest PRIVATE -Wno-stringop-truncation)
    endif()
    target_compile_options(msvcrt_winetest PRIVATE -Wno-format)
endif()

set_module_type(msvcrt_winetest win32cui)
add_importlibs(msvcrt_winetest advapi32 msvcrt kernel32)
target_link_libraries(msvcrt_winetest oldnames)

if(MSVC)
    add_importlibs(msvcrt_winetest ntdll)
endif()

add_pch(msvcrt_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET msvcrt_winetest)
