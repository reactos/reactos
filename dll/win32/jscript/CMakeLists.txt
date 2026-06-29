
spec2def(jscript.dll jscript.spec)

list(APPEND SOURCE
    activex.c
    array.c
    arraybuf.c
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
    set.c
    string.c
    vbarray.c)

BISON_TARGET(cc_parser cc_parser.y ${CMAKE_CURRENT_BINARY_DIR}/cc_parser.tab.c)
BISON_TARGET(parser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    ${BISON_cc_parser_OUTPUTS}
    ${BISON_parser_OUTPUTS})

list(APPEND jscript_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/jscript.rgs)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS "${jscript_rc_deps}")

add_library(jscript MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/jscript.def)

if(MSVC)
    # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
    # Disable warning C4267: '=': conversion from 'size_t' to 'WCHAR', possible loss of data
    target_compile_options(jscript PRIVATE /wd4146 /wd4267)
endif()

add_idl_headers(jscript_idlheader jscript_classes.idl)
add_idl_headers(jscript_jsdisp_header jsdisp.idl)
add_idl_reg_scripts(jscript registry jscript_classes.idl)
add_idl_reg_scripts(jscript regtypelib jsglobal.idl)
add_dependencies(jscript jscript_idlheader jscript_jsdisp_header stdole2)
set_module_type(jscript win32dll)
target_link_libraries(jscript uuid wine oldnames wine_dll_register)
add_importlibs(jscript user32 ole32 oleaut32 advapi32 msvcrt kernel32 ntdll)
add_pch(jscript precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET jscript DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(jscript) # CORE-5743: No ARRAY_SIZE nor CONST_VTABLE
