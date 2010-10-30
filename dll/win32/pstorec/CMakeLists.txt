
ADD_TYPELIB(pstorec_tlb pstorec_tlb.idl)
add_dependencies(pstorec_tlb stdole2)

add_definitions(-D__WINESRC__)

spec2def(pstorec.dll pstorec.spec)

add_library(pstorec SHARED pstorec.c rsrc.rc ${CMAKE_CURRENT_BINARY_DIR}/pstorec.def)

set_module_type(pstorec win32dll)

target_link_libraries(pstorec
    uuid
    wine)

add_importlibs(pstorec msvcrt kernel32 ntdll)

add_dependencies(pstorec pstorec_tlb pstorec_def)
add_cab_target(pstorec 1)
