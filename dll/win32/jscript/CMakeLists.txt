
add_typelib(jsglobal.idl)

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
    date.c
    dispex.c
    engine.c
    error.c
    function.c
    global.c
    jscript.c
    jscript_main.c
    jsutils.c
    lex.c
    math.c
    number.c
    object.c
    parser.tab.c
    regexp.c
    string.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/jscript.def)

add_library(jscript SHARED ${SOURCE})
set_module_type(jscript win32dll)
target_link_libraries(jscript wine)
add_importlibs(jscript user32 ole32 oleaut32 advapi32 msvcrt kernel32 ntdll)
add_pch(jscript jscript.h)
# jsglobal.tlb needs stdole2.tlb
add_dependencies(jscript stdole2)
add_cd_file(TARGET jscript DESTINATION reactos/system32 FOR all)
set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/jsglobal.tlb)
