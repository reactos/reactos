
add_definitions(-D__WINESRC__)
spec2def(pstorec.dll pstorec.spec)

list(APPEND SOURCE
    pstorec.c
    ${CMAKE_CURRENT_BINARY_DIR}/pstorec_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/pstorec.def)

add_typelib(pstorec_tlb.idl)
set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pstorec_tlb.tlb)
add_library(pstorec MODULE ${SOURCE} rsrc.rc)
set_module_type(pstorec win32dll)
target_link_libraries(pstorec uuid wine)
add_importlibs(pstorec msvcrt kernel32 ntdll)
add_dependencies(pstorec stdole2)
add_cd_file(TARGET pstorec DESTINATION reactos/system32 FOR all)
