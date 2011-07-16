
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_typelib(std_ole_v2 std_ole_v2.idl)

spec2def(stdole2.tlb stdole2.tlb.spec)

list(APPEND SOURCE
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/stdole2.def)

add_library(stdole2.tlb SHARED ${SOURCE})

set_entrypoint(stdole2.tlb 0)
set_target_properties(stdole2.tlb PROPERTIES SUFFIX "")
add_dependencies(stdole2.tlb std_ole_v2)
add_cd_file(TARGET stdole2.tlb DESTINATION reactos/system32 FOR all)
