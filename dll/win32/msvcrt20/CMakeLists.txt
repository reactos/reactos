


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)
add_definitions(-DCRTDLL)

spec2def(msvcrt20 ${CMAKE_CURRENT_SOURCE_DIR}/msvcrt20.spec ${CMAKE_CURRENT_BINARY_DIR}/msvcrt20.def)

add_library(msvcrt20 SHARED msvcrt20.c)

set_target_properties(msvcrt20 PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(msvcrt20
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt20.def
    wine
    mingw_common
    -lmsvcrt)

add_dependencies(msvcrt20 msvcrt20_def psdk)
