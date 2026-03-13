
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)
remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_definitions(-D__WINESRC__ -DWINETEST_USE_DBGSTR_LONGLONG)

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

if(_WINKD_)
    add_definitions(-D_WINKD_)
endif()

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
    sync.c
    testlist.c
    thread.c
    threadpool.c
    time.c
    unwind.c
    virtual.c
    wow64.c
)

if(MSVC AND ARCH STREQUAL "amd64")
    add_asm_files(ntdll_winetest_asm msvc-x64.asm)
    list(APPEND SOURCE ${ntdll_winetest_asm})
endif()

add_executable(ntdll_winetest
    ${SOURCE})

target_link_libraries(ntdll_winetest pseh)

if(MSVC)
    target_compile_options(ntdll_winetest PRIVATE
        /wd4024 # warning C4024: 'function': different types for formal and actual parameter 1
        /wd4047 # warning C4047: 'function': 'type' differs in levels of indirection from 'type'
        /wd4101 # warning C4101: 'variable': unreferenced local variable
        /wd4319 # warning C4319: '~': zero extending 'unsigned int' to 'UINT_PTR' of greater size
        /wd4334 # warning C4334: '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
    )
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(ntdll_winetest PRIVATE "-Wno-format")
    target_compile_options(ntdll_winetest PRIVATE "-Wno-int-conversion")
    target_compile_options(ntdll_winetest PRIVATE "-Wno-unused-variable")
endif()

set_module_type(ntdll_winetest win32cui)
add_importlibs(ntdll_winetest user32 ole32 advapi32 advapi32_vista msvcrt kernel32 ntdll)
add_rostests_file(TARGET ntdll_winetest)
target_compile_definitions(ntdll_winetest PRIVATE wcsnicmp=_wcsnicmp)
