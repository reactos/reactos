
add_definitions(
    -D__WINESRC__
    -D_DLL -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

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
    version.rc
    riched20.def)

if(NOT MSVC)
    set_source_files_properties(riched20.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(riched20 win32dll)

target_link_libraries(riched20
    wine
    uuid)

if(MSVC)
    set_target_properties(riched20 PROPERTIES COMPILE_FLAGS "/FIwine/typeof.h")
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

add_cab_target(riched20 1)
