
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(sxs.dll sxs.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cache.c
    name.c
    sxs.c
    ${CMAKE_CURRENT_BINARY_DIR}/sxs_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/sxs.def)

add_library(sxs SHARED ${SOURCE})
set_module_type(sxs win32dll ENTRYPOINT 0 )
target_link_libraries(sxs wine)
add_importlibs(sxs oleaut32 ole32 kernel32 ntdll)
add_dependencies(sxs psdk)
add_cd_file(TARGET sxs DESTINATION reactos/system32 FOR all)
