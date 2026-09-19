
add_definitions(-D__ROS_LONG64__)
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    reader.c
    writer.c
    testlist.c)

add_executable(xmllite_winetest ${SOURCE})
if(MSVC)
    # warning C4566: character represented by universal-character-name cannot be represented in the current code page
    target_compile_options(xmllite_winetest PRIVATE /wd4566)
endif()
set_module_type(xmllite_winetest win32cui)
add_importlibs(xmllite_winetest xmllite ole32 msvcrt kernel32)
add_rostests_file(TARGET xmllite_winetest)
