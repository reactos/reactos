
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    activex.c
    caller.c
    jscript.c
    run.c
    testlist.c)

list(APPEND jscript_winetest_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/api.js
    ${CMAKE_CURRENT_SOURCE_DIR}/cc.js
    ${CMAKE_CURRENT_SOURCE_DIR}/lang.js
    ${CMAKE_CURRENT_SOURCE_DIR}/regexp.js
    ${CMAKE_CURRENT_SOURCE_DIR}/sunspider-regexp-dna.js
    ${CMAKE_CURRENT_SOURCE_DIR}/sunspider-string-base64.js
    ${CMAKE_CURRENT_SOURCE_DIR}/sunspider-string-validate-input.js)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS "${jscript_winetest_rc_deps}")
add_executable(jscript_winetest ${SOURCE} rsrc.rc)
set_module_type(jscript_winetest win32cui)
add_importlibs(jscript_winetest ole32 oleaut32 advapi32 msvcrt kernel32)
add_rostests_file(TARGET jscript_winetest)
