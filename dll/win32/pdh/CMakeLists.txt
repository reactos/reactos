
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(pdh ${CMAKE_CURRENT_SOURCE_DIR}/pdh.spec ${CMAKE_CURRENT_BINARY_DIR}/pdh.def)

add_library(pdh SHARED pdh_main.c)

set_entrypoint(pdh 0)

target_link_libraries(pdh
    ${CMAKE_CURRENT_BINARY_DIR}/pdh.def
    wine)

add_importlibs(pdh kernel32 ntdll)
add_dependencies(pdh pdh_def)
