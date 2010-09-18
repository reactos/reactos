


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)
add_definitions(-DCRTDLL)

spec2def(msvcrt40 ${CMAKE_CURRENT_SOURCE_DIR}/msvcrt40.spec ${CMAKE_CURRENT_BINARY_DIR}/msvcrt40.def)

add_library(msvcrt40 SHARED msvcrt40.c)

set_target_properties(msvcrt40 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(msvcrt40
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt40.def
    wine
    mingw_common
    -lmsvcrt)

add_dependencies(msvcrt40 msvcrt40_def psdk)
