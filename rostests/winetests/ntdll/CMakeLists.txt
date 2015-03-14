
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)

remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)

add_definitions(-D__WINESRC__)

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
    time.c
    testlist.c)

add_executable(ntdll_winetest ${SOURCE})
set_module_type(ntdll_winetest win32cui)
add_importlibs(ntdll_winetest user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ntdll_winetest DESTINATION reactos/bin FOR all)

if(NOT MSVC)
    allow_warnings(ntdll_winetest)
    add_target_compile_flags(ntdll_winetest "-Wno-format")
endif()
