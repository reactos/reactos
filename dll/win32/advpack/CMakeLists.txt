


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

file(GLOB_RECURSE SOURCE *.c)

spec2def(advpack ${CMAKE_CURRENT_SOURCE_DIR}/advpack.spec ${CMAKE_CURRENT_BINARY_DIR}/advpack.def)

add_library(advpack SHARED ${SOURCE})

set_target_properties(advpack PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(advpack
    ${CMAKE_CURRENT_BINARY_DIR}/advpack.def
    wine
    mingw_common
    -lole32
    -lsetupapi
    -lversion
    -ladvapi32)

add_dependencies(advpack advpack_def psdk buildno_header)
