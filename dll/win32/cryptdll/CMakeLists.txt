


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(cryptdll ${CMAKE_CURRENT_SOURCE_DIR}/cryptdll.spec ${CMAKE_CURRENT_BINARY_DIR}/cryptdll.def)

add_library(cryptdll SHARED cryptdll.c)

set_target_properties(cryptdll PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(cryptdll
    ${CMAKE_CURRENT_BINARY_DIR}/cryptdll.def
    wine
    mingw_common)

add_dependencies(cryptdll cryptdll_def psdk)
