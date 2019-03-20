
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

add_definitions(
    -D__WINESRC__
    -D_WINE
    -DCOM_NO_WINDOWS_H)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msxml4.dll msxml4.spec)
add_typelib(msxml4_tlb.idl)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/msxml4.def)

add_library(msxml4 MODULE ${SOURCE} rsrc.rc)
set_module_type(msxml4 win32dll)
set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msxml4_tlb.tlb)
target_link_libraries(msxml4 wine)
add_importlibs(msxml4 msxml3 msvcrt kernel32)
add_dependencies(msxml4 stdole2)
add_cd_file(TARGET msxml4 DESTINATION reactos/system32 FOR all)
