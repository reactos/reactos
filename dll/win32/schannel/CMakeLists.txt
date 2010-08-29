
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

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
