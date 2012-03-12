
add_typelib(jsglobal.idl)

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
    ${CMAKE_CURRENT_BINARY_DIR}/jscript.def)

add_library(jscript SHARED
    ${SOURCE}
    rsrc.rc)

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

add_pch(jscript jscript.h)
# jsglobal.tlb needs stdole2.tlb
add_dependencies(jscript stdole2)
add_cd_file(TARGET jscript DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(jscript)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/jsglobal.tlb)
