
add_typelib(jsglobal jsglobal.idl)

add_dependencies(jsglobal stdole2)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_USE_MATH_DEFINES
    -Disinf=!_finite
    -Disnan=_isnan)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(jscript.dll jscript.spec)

list(APPEND SOURCE
    activex.c
    date.c
    dispex.c
    engine.c
    error.c
    jscript.c
    jscript_main.c
    jsutils.c
    lex.c
    parser.tab.c
    math.c
    number.c
    object.c
    regexp.c
    string.c
    array.c
    bool.c
    function.c
    global.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/jscript.def)

add_library(jscript SHARED ${SOURCE})

set_module_type(jscript win32dll)

target_link_libraries(jscript wine)
if(MSVC)
    target_link_libraries(jscript uuid)
endif()

add_importlibs(jscript
    msvcrt
    user32
    ole32
    oleaut32
    advapi32
    kernel32
    ntdll)

add_dependencies(jscript jsglobal)
add_cd_file(TARGET jscript DESTINATION reactos/system32 FOR all)
