
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(tapi32.dll tapi32.spec)

list(APPEND SOURCE
    assisted.c
    internal.c
    line.c
    phone.c
    ${CMAKE_CURRENT_BINARY_DIR}/tapi32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/tapi32.def)

add_library(tapi32 SHARED ${SOURCE})

set_module_type(tapi32 win32dll ENTRYPOINT 0 )

target_link_libraries(tapi32 wine)

add_importlibs(tapi32 advapi32 kernel32 ntdll)
add_dependencies(tapi32 psdk)

add_cd_file(TARGET tapi32 DESTINATION reactos/system32 FOR all)
