
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(oleacc.dll oleacc.spec)

add_library(oleacc SHARED main.c oleacc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/oleacc.def)

set_entrypoint(oleacc 0)

target_link_libraries(oleacc wine)

add_importlibs(oleacc
    user32
    kernel32
    ntdll)


add_cab_target(oleacc 1)