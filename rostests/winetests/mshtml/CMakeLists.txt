
add_definitions(-DUSE_WINE_TODOS)
add_idl_Headers(test_tlb_header test_tlb.idl)
add_typelib(test_tlb.idl)

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
    testlist.c
    xmlhttprequest.c)

add_executable(mshtml_winetest ${SOURCE} rsrc.rc)

list(APPEND mshtml_winetest_rc_deps
    ${CMAKE_CURRENT_BINARY_DIR}/test_tlb.tlb
    ${CMAKE_CURRENT_SOURCE_DIR}/blank.html
    ${CMAKE_CURRENT_SOURCE_DIR}/exectest.html
    ${CMAKE_CURRENT_SOURCE_DIR}/jstest.html
    ${CMAKE_CURRENT_SOURCE_DIR}/nav_test.html
    ${CMAKE_CURRENT_SOURCE_DIR}/vbtest.html
    ${CMAKE_CURRENT_SOURCE_DIR}/events.html
    ${CMAKE_CURRENT_SOURCE_DIR}/externscr.js)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS "${mshtml_winetest_rc_deps}")
target_link_libraries(mshtml_winetest uuid strmiids)
set_module_type(mshtml_winetest win32cui)
add_importlibs(mshtml_winetest wininet ole32 oleaut32 user32 gdi32 urlmon advapi32 msvcrt kernel32)
add_dependencies(mshtml_winetest test_tlb_header stdole2)
add_cd_file(TARGET mshtml_winetest DESTINATION reactos/bin FOR all)
