
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

add_definitions(
    -D__WINESRC__
    -D_WINE
    -DCOM_NO_WINDOWS_H)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msxml6.dll msxml6.spec)
add_typelib(msxml6_tlb.idl)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/msxml6.def)

add_library(msxml6 MODULE ${SOURCE} rsrc.rc)
set_module_type(msxml6 win32dll)
set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msxml6_tlb.tlb)
target_link_libraries(msxml6 wine)
add_importlibs(msxml6 msxml3 msvcrt kernel32)
add_dependencies(msxml6 stdole2)
add_cd_file(TARGET msxml6 DESTINATION reactos/system32 FOR all)
