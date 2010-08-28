
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(mssign32 ${CMAKE_CURRENT_SOURCE_DIR}/mssign32.spec ${CMAKE_CURRENT_BINARY_DIR}/mssign32.def)

add_library(mssign32 SHARED mssign32_main.c)

set_target_properties(mssign32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(mssign32
    ${CMAKE_CURRENT_BINARY_DIR}/mssign32.def
    wine
    mingw_common)

add_dependencies(mssign32 mssign32_def psdk)
