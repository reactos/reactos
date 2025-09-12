
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    reader.c
    writer.c
    testlist.c)

add_executable(xmllite_winetest ${SOURCE})
set_module_type(xmllite_winetest win32cui)
add_importlibs(xmllite_winetest xmllite ole32 msvcrt kernel32)
add_rostests_file(TARGET xmllite_winetest)
