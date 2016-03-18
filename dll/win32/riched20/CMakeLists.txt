
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
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
    undo.c
    wrap.c
    writer.c
    editor.h)

if(MSVC)
    if(ARCH STREQUAL "i386")
        list(APPEND SOURCE msvc-thiscall.c)
    endif()
    set_source_files_properties(txthost.c txtsrv.c PROPERTIES COMPILE_FLAGS "/FImsvc.h")
    list(APPEND ADDITIONAL_SOURCE txthost.c txtsrv.c)
else()
    list(APPEND SOURCE txthost.c txtsrv.c)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    #FIXME: http://llvm.org/bugs/show_bug.cgi?id=19027
    set_property(SOURCE txthost.c txtsrv.c APPEND_STRING PROPERTY COMPILE_FLAGS " -no-integrated-as")
endif()

list(APPEND ADDITIONAL_SOURCE
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/riched20.def)

list(APPEND riched20_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/riched_tom.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/riched_tom.tlb)

set_source_files_properties(version.rc PROPERTIES OBJECT_DEPENDS "${riched20_rc_deps}")

add_library(riched20 SHARED ${SOURCE} ${ADDITIONAL_SOURCE})
add_typelib(riched_tom.idl)
add_dependencies(riched20 stdole2)
set_module_type(riched20 win32dll)
target_link_libraries(riched20 wine uuid)
add_importlibs(riched20 ole32 oleaut32 usp10 imm32 user32 gdi32 msvcrt kernel32 ntdll)
add_pch(riched20 editor.h SOURCE)
add_cd_file(TARGET riched20 DESTINATION reactos/system32 FOR all)
