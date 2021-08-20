
list(APPEND SOURCE
    info.c
    install.c
    testlist.c)

add_executable(version_winetest ${SOURCE} version.rc)
set_module_type(version_winetest win32cui)
add_importlibs(version_winetest shell32 version msvcrt kernel32)

target_compile_definitions(version_winetest PRIVATE USE_WINE_TODOS)

if(MSVC)
    add_importlibs(version_winetest ntdll)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(version_winetest PRIVATE -Wno-format-overflow)
endif()

add_rostests_file(TARGET version_winetest)
