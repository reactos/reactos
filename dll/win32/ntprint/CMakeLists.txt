include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(ntprint.dll ntprint.spec)

list(APPEND SOURCE
    ntprint.c
    ntprint.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntprint_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/ntprint.def)

add_library(ntprint SHARED ${SOURCE})
set_module_type(ntprint win32dll)
target_link_libraries(ntprint wine)
add_importlibs(ntprint winspool msvcrt kernel32 ntdll)
add_dependencies(ntprint psdk)
add_cd_file(TARGET ntprint DESTINATION reactos/system32 FOR all)
