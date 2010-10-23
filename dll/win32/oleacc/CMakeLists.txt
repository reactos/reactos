
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(oleacc ${CMAKE_CURRENT_SOURCE_DIR}/oleacc.spec ${CMAKE_CURRENT_BINARY_DIR}/oleacc.def)

add_library(oleacc SHARED main.c oleacc.rc)

set_entrypoint(oleacc 0)

target_link_libraries(oleacc
    ${CMAKE_CURRENT_BINARY_DIR}/oleacc.def
    wine)

add_importlibs(oleacc
    user32
    kernel32
    ntdll)

add_dependencies(oleacc oleacc_def)
add_livecd_target(oleacc reactos/system32)