
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(olepro32 ${CMAKE_CURRENT_SOURCE_DIR}/olepro32.spec ${CMAKE_CURRENT_BINARY_DIR}/olepro32.def)

add_library(olepro32 SHARED olepro32stubs.c version.rc)

set_entrypoint(olepro32 0)

target_link_libraries(olepro32
    ${CMAKE_CURRENT_BINARY_DIR}/olepro32.def
    wine)

add_importlibs(olepro32
    oleaut32
    kernel32
    ntdll)

add_dependencies(olepro32 olepro32_def)
add_cab_target(olepro32 1)