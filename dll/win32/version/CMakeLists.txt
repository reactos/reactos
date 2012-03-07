
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(version.dll version.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    info.c
    install.c
    resource.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/version.def)

add_library(version SHARED ${SOURCE})

set_module_type(version win32dll ENTRYPOINT 0 )

target_link_libraries(version wine)
add_importlibs(version lz32 kernel32 ntdll)
add_dependencies(version psdk)
add_cd_file(TARGET version DESTINATION reactos/system32 FOR all)

