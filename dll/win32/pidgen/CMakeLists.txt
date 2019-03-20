
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(pidgen.dll pidgen.spec)

list(APPEND SOURCE
    main.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/pidgen_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/pidgen.def)

add_library(pidgen MODULE ${SOURCE})
set_module_type(pidgen win32dll)
target_link_libraries(pidgen wine)
add_importlibs(pidgen msvcrt kernel32 ntdll)
add_cd_file(TARGET pidgen DESTINATION reactos/system32 FOR all)
