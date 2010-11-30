include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

set_rc_compiler()
spec2def(ntprint.dll ntprint.spec)

list(APPEND SOURCE
    ntprint.c
    ntprint.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntprint.def)

add_library(ntprint SHARED ${SOURCE})

set_module_type(ntprint win32dll)

target_link_libraries(ntprint wine)

add_importlibs(ntprint winspool msvcrt kernel32 ntdll)
add_dependencies(ntprint psdk)
add_cab_target(ntprint 1)
