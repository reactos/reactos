
list(APPEND SOURCE
    mimeintl.c
    mimeole.c)

add_executable(inetcomm_winetest ${SOURCE})
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(inetcomm_winetest PRIVATE -Wno-discarded-qualifiers)
endif()
set_module_type(inetcomm_winetest win32cui)
target_compile_definitions(inetcomm_winetest PRIVATE USE_WINE_TODOS strcasecmp=_stricmp)
target_link_libraries(inetcomm_winetest winetestlib)
add_importlibs(inetcomm_winetest inetcomm oleaut32 ole32 urlmon msvcrt kernel32)
add_rostests_file(TARGET inetcomm_winetest)
