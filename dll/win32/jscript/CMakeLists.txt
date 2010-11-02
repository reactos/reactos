
ADD_TYPELIB(jsglobal jsglobal.idl)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_DLL
    -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(jscript SHARED
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
    jscript.def)

if(NOT MSVC)
    set_source_files_properties(jscript.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(jscript win32dll)

target_link_libraries(jscript wine)

add_importlibs(jscript
    msvcrt
    user32
    ole32
    oleaut32
    advapi32
    kernel32
    ntdll)

add_dependencies(jscript jsglobal stdole2)
add_cab_target(jscript 1)
