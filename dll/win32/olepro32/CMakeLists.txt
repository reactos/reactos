
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_typelib(olepro.idl)

list(APPEND SOURCE
    olepro32stubs.c)

list(APPEND olepro32_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/olepro_t.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/olepro.tlb)

set_source_files_properties(version.rc PROPERTIES OBJECT_DEPENDS "${olepro32_rc_deps}")

add_library(olepro32 MODULE ${SOURCE} version.rc)
spec2def(olepro32 olepro32.dll olepro32.spec)

add_dependencies(olepro32 stdole2)
set_module_type(olepro32 win32dll)
target_link_libraries(olepro32 wine)
add_importlibs(olepro32 oleaut32 msvcrt kernel32 ntdll)
add_cd_file(TARGET olepro32 DESTINATION reactos/system32 FOR all)
