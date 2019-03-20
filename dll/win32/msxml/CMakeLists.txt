
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

add_definitions(
    -D__WINESRC__
    -D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msxml.dll msxml.spec)
add_typelib(msxml_tlb.idl)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/msxml.def)

add_library(msxml MODULE ${SOURCE} rsrc.rc)
set_module_type(msxml win32dll)
set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msxml_tlb.tlb)
target_link_libraries(msxml wine)
add_importlibs(msxml msxml3 msvcrt kernel32)
add_dependencies(msxml stdole2)
add_cd_file(TARGET msxml DESTINATION reactos/system32 FOR all)
