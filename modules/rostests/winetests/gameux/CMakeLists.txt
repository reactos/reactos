
list(APPEND SOURCE
    gameexplorer.c
    gamestatistics.c)

add_executable(gameux_winetest 
    ${SOURCE} 
    rsrc.rc
    testlist.c)

target_compile_definitions(gameux_winetest PRIVATE USE_WINE_TODOS)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(gameux_winetest PRIVATE -Wno-format-overflow)
endif()

set_module_type(gameux_winetest win32cui)
target_link_libraries(gameux uuid wine)
add_importlibs(gameux_winetest shlwapi oleaut32 ole32 advapi32 shell32 user32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET gameux_winetest)
