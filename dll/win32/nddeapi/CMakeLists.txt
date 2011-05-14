
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(nddeapi.dll nddeapi.spec)

list(APPEND SOURCE
    nddeapi.c
    ${CMAKE_CURRENT_BINARY_DIR}/nddeapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/nddeapi.def)

add_library(nddeapi SHARED ${SOURCE})
set_entrypoint(nddeapi 0)

target_link_libraries(nddeapi wine)

add_importlibs(nddeapi kernel32 ntdll)
add_dependencies(nddeapi psdk)
add_cd_file(TARGET nddeapi DESTINATION reactos/system32 FOR all)