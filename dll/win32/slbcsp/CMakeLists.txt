
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(slbcsp ${CMAKE_CURRENT_SOURCE_DIR}/slbcsp.spec ${CMAKE_CURRENT_BINARY_DIR}/slbcsp.def)

add_library(slbcsp SHARED main.c)

set_target_properties(slbcsp PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(slbcsp
    ${CMAKE_CURRENT_BINARY_DIR}/slbcsp.def
    wine
    mingw_common)

add_dependencies(slbcsp slbcsp_def psdk)
