
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

ADD_TYPELIB(std_ole_v2 std_ole_v2.idl)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/stdole2.tlb.def
    COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/stdole2.tlb.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/stdole2.tlb.spec --filename stdole2.tlb
    DEPENDS native-winebuild)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/stdole2.tlb.def PROPERTIES GENERATED TRUE)
add_custom_target(stdole2.tlb_def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/stdole2.tlb.def)

add_library(stdole2.tlb SHARED rsrc.rc)

set_entrypoint(stdole2.tlb 0)
set_target_properties(stdole2.tlb PROPERTIES SUFFIX "")

target_link_libraries(stdole2.tlb ${CMAKE_CURRENT_BINARY_DIR}/stdole2.tlb.def)

add_dependencies(stdole2.tlb std_ole_v2 stdole2.tlb_def)
add_livecd_target(stdole2.tlb reactos/system32)
