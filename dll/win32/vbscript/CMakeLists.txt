
spec2def(vbscript.dll vbscript.spec)

list(APPEND SOURCE
    compile.c
    global.c
    interp.c
    lex.c
    regexp.c
    utils.c
    vbdisp.c
    vbregexp.c
    vbscript.c
    vbscript_main.c)

BISON_TARGET(parser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    ${BISON_parser_OUTPUTS})

list(APPEND vbscript_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/vbscript.rgs)

set_source_files_properties(vbscript.rc PROPERTIES OBJECT_DEPENDS "${vbscript_rc_deps}")
add_idl_headers(vbscript_idlheader vbscript_classes.idl vbsglobal.idl vbsregexp55.idl)

add_library(vbscript MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    vbscript.rc
    ${CMAKE_CURRENT_BINARY_DIR}/vbscript.def)

if(MSVC)
    # Disable warning C4267: '=': conversion from 'size_t' to 'WCHAR', possible loss of data
    target_compile_options(vbscript PRIVATE /wd4267)
endif()

add_idl_reg_scripts(vbscript registry vbscript_classes.idl)
add_idl_reg_scripts(vbscript regtypelib vbsglobal.idl vbsregexp10.idl vbsregexp55.idl)
set_module_type(vbscript win32dll)
target_link_libraries(vbscript uuid wine oldnames wine_dll_register wine_dll_canunload)
add_importlibs(vbscript oleaut32 ole32 user32 msvcrt kernelbase kernel32_vista kernel32 ntdll)
add_dependencies(vbscript vbscript_idlheader stdole2)
add_pch(vbscript precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET vbscript DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(vbscript) # CORE-5743: No ARRAY_SIZE and CONST_VTABLE
