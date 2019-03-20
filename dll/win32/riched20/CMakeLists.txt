
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(riched20.dll riched20.spec ADD_IMPORTLIB)

list(APPEND SOURCE
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
    precomp.h)

list(APPEND riched20_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/riched_tom.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/riched_tom.tlb)
set_source_files_properties(version.rc PROPERTIES OBJECT_DEPENDS "${riched20_rc_deps}")

add_library(riched20 MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/riched20.def)

add_typelib(riched_tom.idl)
add_dependencies(riched20 stdole2)
set_module_type(riched20 win32dll)
target_link_libraries(riched20 wine uuid)
add_importlibs(riched20 ole32 oleaut32 usp10 imm32 user32 gdi32 msvcrt kernel32 ntdll)
add_pch(riched20 precomp.h SOURCE)
add_cd_file(TARGET riched20 DESTINATION reactos/system32 FOR all)
