
spec2def(pstorec.dll pstorec.spec)

list(APPEND SOURCE
    pstorec.c
    ${CMAKE_CURRENT_BINARY_DIR}/pstorec_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/pstorec.def)

add_library(pstorec MODULE ${SOURCE})
set_module_type(pstorec win32dll)
add_idl_reg_scripts(pstorec regtypelib pstorec_tlb.idl)
target_link_libraries(pstorec uuid wine wine_dll_register wine_dll_canunload)
add_importlibs(pstorec msvcrt kernel32 ntdll)
add_dependencies(pstorec stdole2)
add_cd_file(TARGET pstorec DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(pstorec) # CORE-5743: No CONST_VTABLE
