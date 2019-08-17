
add_definitions(-DUSE_WINE_TODOS)

if(GCC AND GCC_VERSION VERSION_GREATER 7)
    add_compile_flags("-Wno-format-overflow")
endif()

list(APPEND SOURCE
    info.c
    install.c
    testlist.c)

add_executable(version_winetest ${SOURCE} version.rc)
set_module_type(version_winetest win32cui)
add_importlibs(version_winetest shell32 version msvcrt kernel32)

if(MSVC)
    add_importlibs(version_winetest ntdll)
endif()

add_rostests_file(TARGET version_winetest)
