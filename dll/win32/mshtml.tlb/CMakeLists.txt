
add_typelib(mshtml_tlb.idl)

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(mshtml.tlb SHARED rsrc.rc)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mshtml_tlb.tlb)

set_module_type(mshtml.tlb module)
set_target_properties(mshtml.tlb PROPERTIES SUFFIX "")
add_dependencies(mshtml.tlb stdole2)
add_cd_file(TARGET mshtml.tlb DESTINATION reactos/system32 FOR all)
