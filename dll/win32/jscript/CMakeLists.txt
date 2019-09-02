
add_definitions(
    -D__WINESRC__
    -D_USE_MATH_DEFINES)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(jscript.dll jscript.spec)

list(APPEND SOURCE
    activex.c
    array.c
    bool.c
    compile.c
    date.c
    decode.c
    dispex.c
    engine.c
    enumerator.c
    error.c
    function.c
    global.c
    jscript.c
    jscript_main.c
    json.c
    jsregexp.c
    jsstr.c
    jsutils.c
    lex.c
    math.c
    number.c
    object.c
    regexp.c
    string.c
    vbarray.c
    precomp.h)

# cc_parser.tab.c/parser.tab.c have been generated with relative file paths...
set_source_files_properties(cc_parser.tab.c parser.tab.c PROPERTIES COMPILE_FLAGS "-UREACTOS_SOURCE_DIR -DREACTOS_SOURCE_DIR=\"\\\".\\\"\"")

list(APPEND jscript_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/jscript.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/jscript_classes.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/jsglobal.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/jsglobal.tlb)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS "${jscript_rc_deps}")

add_library(jscript MODULE
    ${SOURCE}
    cc_parser.tab.c
    parser.tab.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/jscript.def)

add_idl_headers(jscript_idlheader jscript_classes.idl)
add_typelib(jsglobal.idl)
add_dependencies(jscript jscript_idlheader stdole2)
set_module_type(jscript win32dll)
target_link_libraries(jscript uuid wine)
add_importlibs(jscript user32 ole32 oleaut32 advapi32 msvcrt kernel32 ntdll)
add_pch(jscript precomp.h SOURCE)
add_cd_file(TARGET jscript DESTINATION reactos/system32 FOR all)
