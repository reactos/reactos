
remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)

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
    scanf.c
    signal.c
    string.c
    time.c)

list(APPEND PCH_SKIP_SOURCE
    printf.c
    testlist.c)

add_executable(msvcrt_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

target_compile_definitions(msvcrt_winetest PRIVATE
    WINETEST_USE_DBGSTR_LONGLONG
    USE_WINE_TODOS
    USE_WIN_SKIP
    _CRT_NONSTDC_NO_DEPRECATE
    _USE_MATH_DEFINES
    WIN32_NO_STATUS=
    __msvcrt_ulong=ULONG)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(msvcrt_winetest PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-format -Wno-overflow>)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(msvcrt_winetest PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-stringop-truncation>)
    endif()
endif()

if(MSVC)
    # 'swscanf' : unknown type field character '\xe1' in format specifier
    target_compile_options(msvcrt_winetest PRIVATE /wd4476)
endif()

set_module_type(msvcrt_winetest win32cui)
add_importlibs(msvcrt_winetest advapi32 msvcrt kernel32 ntdll)
target_link_libraries(msvcrt_winetest oldnames)

add_pch(msvcrt_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET msvcrt_winetest)
