
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

ADD_TYPELIB(std_ole_v1 std_ole_v1.idl)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/stdole32.tlb.def
    COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/stdole32.tlb.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/stdole32.tlb.spec --filename stdole32.tlb
    DEPENDS native-winebuild)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/stdole32.tlb.def PROPERTIES GENERATED TRUE)
add_custom_target(stdole32.tlb_def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/stdole32.tlb.def)

add_library(stdole32.tlb SHARED rsrc.rc)

set_entrypoint(stdole32.tlb 0)
set_target_properties(stdole32.tlb PROPERTIES SUFFIX "")



add_dependencies(stdole32.tlb std_ole_v1 stdole32.tlb_def)
add_cab_target(stdole32.tlb 1)
