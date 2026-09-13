
list(APPEND SOURCE
    generated.c
    misc.c
    protocol.c
    sec_mgr.c
    stream.c
    uri.c
    url.c)

add_executable(urlmon_winetest ${SOURCE})
target_compile_definitions(urlmon_winetest PRIVATE USE_WINE_TODOS)
target_link_libraries(urlmon_winetest uuid winetestlib)
target_include_directories(urlmon_winetest BEFORE PRIVATE ${REACTOS_SOURCE_DIR}/sdk/include/wine)
set_module_type(urlmon_winetest win32cui)
add_importlibs(urlmon_winetest urlmon wininet ole32 oleaut32 shlwapi user32 advapi32 msvcrt kernel32)
add_rostests_file(TARGET urlmon_winetest)
