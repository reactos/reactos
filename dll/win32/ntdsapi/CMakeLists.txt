
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(ntdsapi ${CMAKE_CURRENT_SOURCE_DIR}/ntdsapi.spec ${CMAKE_CURRENT_BINARY_DIR}/ntdsapi.def)

add_library(ntdsapi SHARED ntdsapi.c)

set_entrypoint(ntdsapi 0)

target_link_libraries(ntdsapi
    ${CMAKE_CURRENT_BINARY_DIR}/ntdsapi.def
    wine)

add_importlibs(ntdsapi user32 kernel32 ntdll)
add_dependencies(ntdsapi ntdsapi_def psdk buildno_header)
