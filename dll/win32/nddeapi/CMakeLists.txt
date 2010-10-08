
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(nddeapi ${CMAKE_CURRENT_SOURCE_DIR}/nddeapi.spec ${CMAKE_CURRENT_BINARY_DIR}/nddeapi.def)

add_library(nddeapi SHARED nddeapi.c)

set_entrypoint(nddeapi 0)

target_link_libraries(nddeapi
    ${CMAKE_CURRENT_BINARY_DIR}/nddeapi.def
    wine)

add_importlibs(nddeapi kernel32 ntdll)
add_dependencies(nddeapi nddeapi_def psdk buildno_header)
