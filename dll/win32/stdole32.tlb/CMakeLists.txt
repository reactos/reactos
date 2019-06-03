
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_typelib(std_ole_v1.idl)

list(APPEND SOURCE
    rsrc.rc)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/std_ole_v1.tlb)
add_library(stdole32.tlb MODULE ${SOURCE})
spec2def(stdole32.tlb stdole32.tlb stdole32.tlb.spec)

set_module_type(stdole32.tlb module)
set_target_properties(stdole32.tlb PROPERTIES SUFFIX "")
add_cd_file(TARGET stdole32.tlb DESTINATION reactos/system32 FOR all)
