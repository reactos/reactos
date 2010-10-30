
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(updspapi.dll updspapi.spec)

add_library(updspapi SHARED main.c
    ${CMAKE_CURRENT_BINARY_DIR}/updspapi.def)

set_entrypoint(updspapi 0)

target_link_libraries(updspapi wine)

add_importlibs(updspapi setupapi kernel32 ntdll)
add_dependencies(updspapi psdk buildno_header)
