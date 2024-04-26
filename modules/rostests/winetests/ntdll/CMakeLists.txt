
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
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
    testlist.c
    threadpool.c
    time.c)

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE
        generated.c)
endif()

add_executable(ntdll_winetest
    ${SOURCE})

target_link_libraries(ntdll_winetest pseh)

if(MSVC AND ARCH STREQUAL "amd64")
    # warning C4334: '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
    target_compile_options(ntdll_winetest PRIVATE /wd4334)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(ntdll_winetest PRIVATE "-Wno-format")
endif()

set_module_type(ntdll_winetest win32cui)
add_importlibs(ntdll_winetest user32 ole32 advapi32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET ntdll_winetest)
