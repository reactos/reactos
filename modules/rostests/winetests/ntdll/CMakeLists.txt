
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__ -DWINETEST_USE_DBGSTR_LONGLONG)

list(APPEND SOURCE
    atom.c
    change.c
    directory.c
    env.c
    error.c
    file.c
    info.c
    large_int.c
    om.c
    path.c
    pipe.c
    port.c
    process.c
    reg.c
    rtl.c
    rtlbitmap.c
    rtlstr.c
    string.c
    time.c
    precomp.h)

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE
        exception.c
        generated.c)
endif()

add_executable(ntdll_winetest ${SOURCE} testlist.c)

if(USE_CLANG_CL OR (NOT MSVC))
    target_compile_options(ntdll_winetest PRIVATE "-Wno-format")
endif()

set_module_type(ntdll_winetest win32cui)
add_importlibs(ntdll_winetest user32 ole32 advapi32 msvcrt kernel32 ntdll)
add_pch(ntdll_winetest precomp.h SOURCE)
add_rostests_file(TARGET ntdll_winetest)
