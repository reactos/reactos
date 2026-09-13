
add_idl_headers(vbscript_wine_test_idlheader vbsregexp55.idl)

list(APPEND SOURCE
    caller.c
    createobj.c
    run.c
    vbscript.c)

list(APPEND vbscript_winetest_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/api.vbs
    ${CMAKE_CURRENT_SOURCE_DIR}/error.vbs
    ${CMAKE_CURRENT_SOURCE_DIR}/lang.vbs
    ${CMAKE_CURRENT_SOURCE_DIR}/regexp.vbs)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS "${vbscript_winetest_rc_deps}")
add_executable(vbscript_winetest ${SOURCE} rsrc.rc)
set_module_type(vbscript_winetest win32cui)
target_compile_definitions(vbscript_winetest PRIVATE USE_WINE_TODOS)
target_link_libraries(vbscript_winetest winetestlib)
add_importlibs(vbscript_winetest ole32 oleaut32 advapi32 msvcrt kernel32)
add_dependencies(vbscript_winetest vbscript_wine_test_idlheader)
add_rostests_file(TARGET vbscript_winetest)
