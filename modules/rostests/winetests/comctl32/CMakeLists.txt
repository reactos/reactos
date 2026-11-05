
remove_definitions(-D_WIN32_WINNT=0x502 -D_WIN32_IE=0x600)

add_definitions(-DUSE_WINE_TODOS -DWINETEST_USE_DBGSTR_LONGLONG)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)

list(APPEND SOURCE
    animate.c
    combo.c
    datetime.c
    dpa.c
    edit.c
    header.c
    imagelist.c
    ipaddress.c
    listbox.c
    listview.c
    misc.c
    monthcal.c
    mru.c
    pager.c
    progress.c
    propsheet.c
    rebar.c
    static.c
    status.c
    syslink.c
    tab.c
    taskdialog.c
    toolbar.c
    tooltips.c
    trackbar.c
    treeview.c
    updown.c)

list(APPEND PCH_SKIP_SOURCE
    button.c
    subclass.c
    testlist.c)

add_executable(comctl32_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    rsrc.rc)

if(MSVC AND ARCH STREQUAL "amd64")
    # warning C4334: '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
    target_compile_options(comctl32_winetest PRIVATE /wd4334)
endif()

if(MSVC)
    # warning C4045: 'large_truncated_80_w': array bounds overflow
    # warning C4047: 'function': 'type' differs in levels of indirection from 'type'
    # warning C4024: different types for formal and actual parameter
    target_compile_options(comctl32_winetest PRIVATE /wd4045 /wd4047 /wd4024)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(comctl32_winetest PRIVATE -Wno-int-conversion -Wno-old-style-declaration)
endif()

target_compile_options(comctl32_winetest PRIVATE $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wno-format>)

set_module_type(comctl32_winetest win32cui)
add_importlibs(comctl32_winetest comctl32 imm32 uxtheme ole32 oleacc oleaut32 user32 gdi32 advapi32 msvcrt kernel32)
target_link_libraries(comctl32_winetest uuid oldnames)

if(MSVC)
    add_importlibs(comctl32_winetest ntdll)
endif()

add_pch(comctl32_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET comctl32_winetest)
