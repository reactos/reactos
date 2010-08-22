
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dciman32 ${CMAKE_CURRENT_SOURCE_DIR}/dciman32.spec ${CMAKE_CURRENT_BINARY_DIR}/dciman32.def)

add_library(dciman32 SHARED dciman_main.c)

set_target_properties(dciman32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(dciman32
    ${CMAKE_CURRENT_BINARY_DIR}/dciman32.def
    wine
    mingw_common)

add_dependencies(dciman32 dciman32_def psdk)
