
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    editor.c
    richole.c
    testlist.c
    txtsrv.c)

if(MSVC)
    set_property(SOURCE editor.c APPEND_STRING PROPERTY COMPILE_FLAGS " /w14189")
endif()

add_executable(riched20_winetest ${SOURCE})
set_module_type(riched20_winetest win32cui)
add_importlibs(riched20_winetest ole32 oleaut32 user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET riched20_winetest DESTINATION reactos/bin FOR all)
