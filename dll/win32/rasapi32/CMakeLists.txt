
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

set_rc_compiler()
spec2def(rasapi32.dll rasapi32.spec)

list(APPEND SOURCE
    rasapi.c
    ${CMAKE_CURRENT_BINARY_DIR}/rasapi32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/rasapi32.def)

add_library(rasapi32 SHARED ${SOURCE})
set_entrypoint(rasapi32 0)

target_link_libraries(rasapi32 wine)

add_importlibs(rasapi32 kernel32 ntdll)
add_importlib_target(rasapi32.spec)
add_dependencies(rasapi32 psdk)
add_cd_file(TARGET rasapi32 DESTINATION reactos/system32 FOR all)
