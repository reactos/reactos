


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(schannel ${CMAKE_CURRENT_SOURCE_DIR}/schannel.spec ${CMAKE_CURRENT_BINARY_DIR}/schannel.def)

add_library(schannel SHARED
    lsamode.c
    schannel_main.c
    usermode.c
    version.rc)

set_target_properties(schannel PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(schannel
    ${CMAKE_CURRENT_BINARY_DIR}/schannel.def
    wine
    -lsecur32
    mingw_common)

add_dependencies(schannel schannel_def psdk)
