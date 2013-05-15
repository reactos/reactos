
add_definitions(-D__ROS_LONG64__)
add_idl_headers(vbscript_wine_test_idlheader vbsregexp55.idl)

list(APPEND SOURCE
    createobj.c
    run.c
    vbscript.c
    testlist.c
    rsrc.rc)

list(APPEND vbscript_winetest_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/api.vbs
    ${CMAKE_CURRENT_SOURCE_DIR}/lang.vbs
    ${CMAKE_CURRENT_SOURCE_DIR}/regexp.vbs)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS "${vbscript_winetest_rc_deps}")

add_executable(vbscript_winetest ${SOURCE})
target_link_libraries(vbscript_winetest wine)
set_module_type(vbscript_winetest win32cui)
add_importlibs(vbscript_winetest ole32 oleaut32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(vbscript_winetest vbscript_wine_test_idlheader)
add_cd_file(TARGET vbscript_winetest DESTINATION reactos/bin FOR all)
