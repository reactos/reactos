
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    pstorec.c)

add_typelib(pstorec_tlb.idl)
set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pstorec_tlb.tlb)

add_library(pstorec MODULE ${SOURCE} rsrc.rc)
spec2def(pstorec pstorec.dll pstorec.spec)

set_module_type(pstorec win32dll)
target_link_libraries(pstorec uuid wine)
add_importlibs(pstorec msvcrt kernel32 ntdll)
add_dependencies(pstorec stdole2)
add_cd_file(TARGET pstorec DESTINATION reactos/system32 FOR all)
