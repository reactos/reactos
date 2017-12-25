
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)
add_definitions(-D__WINESRC__ -DWINETEST_USE_DBGSTR_LONGLONG)

list(APPEND SOURCE
    atom.c
    change.c
    directory.c
    env.c
    error.c
    exception.c
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
    ntdll_test.h)

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE generated.c)
endif()

add_executable(ntdll_winetest ${SOURCE} testlist.c)

if(USE_CLANG_CL OR (NOT MSVC))
    add_target_compile_flags(ntdll_winetest "-Wno-format")
endif()

set_module_type(ntdll_winetest win32cui)
add_importlibs(ntdll_winetest user32 ole32 advapi32 msvcrt kernel32 ntdll)
add_pch(ntdll_winetest ntdll_test.h SOURCE NO_UNITY_BUILD)
add_rostests_file(TARGET ntdll_winetest)
