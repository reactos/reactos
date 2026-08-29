
add_definitions(-DUSE_WINE_TODOS)
add_idl_Headers(test_tlb_header test_tlb.idl)

list(APPEND SOURCE
    activex.c
    dom.c
    events.c
    htmldoc.c
    htmllocation.c
    misc.c
    protocol.c
    script.c
    style.c
    xmlhttprequest.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    testlist.c)

add_executable(mshtml_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    rsrc.rc)

if(MSVC)
    # warning C4034: sizeof returns 0
    target_compile_options(mshtml_winetest PRIVATE /wd4034)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(mshtml_winetest PRIVATE -Wno-discarded-qualifiers)
endif()

list(APPEND mshtml_winetest_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/winetest.js
    ${CMAKE_CURRENT_SOURCE_DIR}/exectest.html
    ${CMAKE_CURRENT_SOURCE_DIR}/asyncscriptload.js
    ${CMAKE_CURRENT_SOURCE_DIR}/jstest.html
    ${CMAKE_CURRENT_SOURCE_DIR}/vbtest.html
    ${CMAKE_CURRENT_SOURCE_DIR}/events.html
    ${CMAKE_CURRENT_SOURCE_DIR}/runscript.html
    ${CMAKE_CURRENT_SOURCE_DIR}/frame.js
    ${CMAKE_CURRENT_SOURCE_DIR}/externscr.js
    ${CMAKE_CURRENT_SOURCE_DIR}/navigation.js
    ${CMAKE_CURRENT_SOURCE_DIR}/xhr.js
    ${CMAKE_CURRENT_SOURCE_DIR}/dom.js
    ${CMAKE_CURRENT_SOURCE_DIR}/es5.js
    ${CMAKE_CURRENT_SOURCE_DIR}/events.js
    ${CMAKE_CURRENT_SOURCE_DIR}/documentmode.js
    ${CMAKE_CURRENT_SOURCE_DIR}/reload.js
    ${CMAKE_CURRENT_SOURCE_DIR}/blank.html
    ${CMAKE_CURRENT_SOURCE_DIR}/blank_ie10.html
    ${CMAKE_CURRENT_SOURCE_DIR}/doc_with_prop.html
    ${CMAKE_CURRENT_SOURCE_DIR}/doc_with_prop_ie9.html
    ${CMAKE_CURRENT_SOURCE_DIR}/iframe.html
    ${CMAKE_CURRENT_SOURCE_DIR}/xhr_iframe.html
    ${CMAKE_CURRENT_SOURCE_DIR}/img.png)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS "${mshtml_winetest_rc_deps}")
add_idl_reg_scripts(mshtml_winetest regtypelib test_tlb.idl)
target_link_libraries(mshtml_winetest oldnames uuid strmiids)
set_module_type(mshtml_winetest win32cui)
add_importlibs(mshtml_winetest wininet ole32 oleaut32 user32 gdi32 urlmon advapi32 shlwapi msvcrt kernel32)
add_dependencies(mshtml_winetest test_tlb_header stdole2)
add_pch(mshtml_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET mshtml_winetest)
