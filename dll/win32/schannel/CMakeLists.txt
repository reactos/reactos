
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(schannel ${CMAKE_CURRENT_SOURCE_DIR}/schannel.spec ${CMAKE_CURRENT_BINARY_DIR}/schannel.def)

add_library(schannel SHARED
    lsamode.c
    schannel_main.c
    usermode.c
    version.rc)

set_module_type(schannel win32dll)

target_link_libraries(schannel
    ${CMAKE_CURRENT_BINARY_DIR}/schannel.def
    wine)

add_importlibs(schannel secur32 kernel32 ntdll)
add_dependencies(schannel schannel_def)
add_cab_target(schannel 1)
