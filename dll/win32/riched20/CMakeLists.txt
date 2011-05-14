
add_definitions(
    -D__WINESRC__
    -D_DLL -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(riched20.dll riched20.spec)
set_rc_compiler()

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
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/riched20.def)

if(MSVC)
    list(APPEND SOURCE msvc-thiscall.c)
    set_source_files_properties(txthost.c txtsrv.c PROPERTIES COMPILE_FLAGS "/FImsvc.h")
endif()

add_library(riched20 SHARED ${SOURCE})
set_module_type(riched20 win32dll)
target_link_libraries(riched20 wine uuid)

add_importlibs(riched20
    msvcrt
    ole32
    oleaut32
    imm32
    user32
    gdi32
    kernel32
    ntdll)

add_importlib_target(riched20.spec)

add_cd_file(TARGET riched20 DESTINATION reactos/system32 FOR all)
