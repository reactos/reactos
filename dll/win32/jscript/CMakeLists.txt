
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_USE_MATH_DEFINES)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

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
    error.c
    function.c
    global.c
    jscript.c
    jscript_main.c
    jsregexp.c
    jsstr.c
    jsutils.c
    lex.c
    math.c
    number.c
    object.c
    parser.tab.c
    regexp.c
    string.c
    vbarray.c
    ${CMAKE_CURRENT_BINARY_DIR}/jscript.def)

add_library(jscript SHARED ${SOURCE} rsrc.rc)
add_idl_headers(jscript_idlheader jscript_classes.idl)

add_typelib(jsglobal.idl)

list(APPEND jscript_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/jscript.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/jscript_classes.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/jsglobal.tlb)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS "${jscript_rc_deps}")

# jsglobal.tlb needs stdole2.tlb
add_dependencies(jscript jscript_idlheader stdole2)


if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(jscript)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()

set_module_type(jscript win32dll)
target_link_libraries(jscript wine)
add_importlibs(jscript user32 ole32 oleaut32 advapi32 msvcrt kernel32 ntdll)
add_pch(jscript jscript.h)
add_cd_file(TARGET jscript DESTINATION reactos/system32 FOR all)
