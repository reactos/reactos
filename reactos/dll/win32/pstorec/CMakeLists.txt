
add_typelib(pstorec_tlb pstorec_tlb.idl)
add_dependencies(pstorec_tlb stdole2)

add_definitions(-D__WINESRC__)

set_rc_compiler()
spec2def(pstorec.dll pstorec.spec GENERATE_STUBS)

list(APPEND SOURCE
    pstorec.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/pstorec_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/pstorec.def)

add_library(pstorec SHARED ${SOURCE})

set_module_type(pstorec win32dll)
target_link_libraries(pstorec uuid wine)
add_importlibs(pstorec msvcrt kernel32 ntdll)
add_dependencies(pstorec pstorec_tlb)
add_cd_file(TARGET pstorec DESTINATION reactos/system32 FOR all)
