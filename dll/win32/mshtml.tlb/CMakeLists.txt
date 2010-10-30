
ADD_TYPELIB(mshtml_tlb mshtml_tlb.idl)
add_dependencies(mshtml_tlb stdole2)

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mshtml.tlb.def
    COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/mshtml.tlb.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/mshtml.tlb.spec --filename mshtml.tlb
    DEPENDS native-winebuild)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/mshtml.tlb.def PROPERTIES GENERATED TRUE)
add_custom_target(mshtml.tlb_def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mshtml.tlb.def)

add_library(mshtml.tlb SHARED rsrc.rc)

set_entrypoint(mshtml.tlb 0)
set_target_properties(mshtml.tlb PROPERTIES SUFFIX "")

target_link_libraries(mshtml.tlb ${CMAKE_CURRENT_BINARY_DIR}/mshtml.tlb.def)

add_dependencies(mshtml.tlb mshtml_tlb mshtml.tlb_def)
add_cab_target(mshtml.tlb 1)
