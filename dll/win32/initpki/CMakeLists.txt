


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(initpki ${CMAKE_CURRENT_SOURCE_DIR}/initpki.spec ${CMAKE_CURRENT_BINARY_DIR}/initpki.def)

add_library(initpki SHARED main.c)

set_target_properties(initpki PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(initpki
    ${CMAKE_CURRENT_BINARY_DIR}/initpki.def
    wine
    mingw_common)

add_dependencies(initpki initpki_def psdk)
