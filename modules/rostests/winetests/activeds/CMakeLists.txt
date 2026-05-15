
list(APPEND SOURCE
    activeds.c
)

add_executable(activeds_winetest ${SOURCE})

target_compile_definitions(activeds_winetest PRIVATE STANDALONE USE_WINE_TODOS)

#if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
#    target_compile_options(activeds_winetest PRIVATE -Wno-format-overflow)
#endif()

set_module_type(activeds_winetest win32cui)
add_importlibs(activeds_winetest activeds ole32 oleaut32 msvcrt kernel32)
add_rostests_file(TARGET activeds_winetest)
