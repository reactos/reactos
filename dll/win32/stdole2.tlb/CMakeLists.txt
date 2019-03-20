
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_typelib(std_ole_v2.idl)
spec2def(stdole2.tlb stdole2.tlb.spec)

list(APPEND SOURCE
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/stdole2.def)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/std_ole_v2.tlb)
add_library(stdole2.tlb MODULE ${SOURCE})
set_module_type(stdole2.tlb module)
set_target_properties(stdole2.tlb PROPERTIES SUFFIX "")
add_cd_file(TARGET stdole2.tlb DESTINATION reactos/system32 FOR all)
