
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

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
