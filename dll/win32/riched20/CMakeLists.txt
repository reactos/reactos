
add_definitions(-D__WINESRC__)
add_definitions(-D_DLL -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(riched20 ${CMAKE_CURRENT_SOURCE_DIR}/riched20.spec ${CMAKE_CURRENT_BINARY_DIR}/riched20.def)

add_library(riched20 SHARED
    caret.c
    clipboard.c
    context.c
    editor.c
    list.c
    paint.c
    para.c
    reader.c
    richole.c
    row.c
    run.c
    string.c
    style.c
    table.c
    txthost.c
    txtsrv.c
    undo.c
    wrap.c
    writer.c
    version.rc)

set_module_type(riched20 win32dll)

target_link_libraries(riched20
    ${CMAKE_CURRENT_BINARY_DIR}/riched20.def
    wine
    uuid)

if(MSVC)
    set_target_properties(portcls PROPERTIES COMPILE_FLAGS "/FIwine/typeof.h")
endif(MSVC)

add_importlibs(riched20
    msvcrt
    ole32
    oleaut32
    imm32
    user32
    gdi32
    kernel32
    ntdll)

add_dependencies(riched20 riched20_def)
add_cab_target(riched20 1)
