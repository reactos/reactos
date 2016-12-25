
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    dictionary.c
    filesystem.c
    testlist.c)

add_executable(scrrun_winetest ${SOURCE})
add_idl_headers(scrrun_idlheader_test scrrun.idl)
add_dependencies(scrrun_winetest scrrun_idlheader_test)
target_link_libraries(scrrun_winetest uuid)
set_module_type(scrrun_winetest win32cui)
add_importlibs(scrrun_winetest ole32 oleaut32 user32 msvcrt kernel32)
add_cd_file(TARGET scrrun_winetest DESTINATION reactos/bin FOR all)
