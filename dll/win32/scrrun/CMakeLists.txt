
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(scrrun.dll scrrun.spec)
add_idl_headers(scrrun_idlheader scrrun.idl)
add_typelib(scrrun.idl)

list(APPEND SOURCE
    dictionary.c
    filesystem.c
    scrrun.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/scrrun_stubs.c)

list(APPEND scrrun_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/scrrun.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/scrrun_tlb.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/scrrun.tlb)

set_source_files_properties(scrrun.rc PROPERTIES OBJECT_DEPENDS "${scrrun_rc_deps}")

add_library(scrrun MODULE
    ${SOURCE}
    guid.c
    scrrun.rc
    ${CMAKE_CURRENT_BINARY_DIR}/scrrun.def)

add_dependencies(scrrun scrrun_idlheader stdole2)
set_module_type(scrrun win32dll)
target_link_libraries(scrrun uuid wine)
add_importlibs(scrrun oleaut32 version advapi32 msvcrt kernel32 ntdll)
add_pch(scrrun precomp.h SOURCE)
add_cd_file(TARGET scrrun DESTINATION reactos/system32 FOR all)
