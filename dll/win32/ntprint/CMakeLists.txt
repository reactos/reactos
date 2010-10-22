
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(ntprint ${CMAKE_CURRENT_SOURCE_DIR}/ntprint.spec ${CMAKE_CURRENT_BINARY_DIR}/ntprint.def)

add_library(ntprint SHARED ntprint.c ntprint.rc)

set_module_type(ntprint win32dll)

target_link_libraries(ntprint
    ${CMAKE_CURRENT_BINARY_DIR}/ntprint.def
    wine)

add_importlibs(ntprint winspool kernel32 ntdll)
add_dependencies(ntprint ntprint_def psdk)
add_livecd_target(ntprint reactos/system32)
