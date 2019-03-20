
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(sccbase.dll sccbase.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/sccbase_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/sccbase.def)

add_library(sccbase MODULE ${SOURCE})
set_module_type(sccbase win32dll)
target_link_libraries(sccbase wine)
add_importlibs(sccbase msvcrt kernel32 ntdll)
add_cd_file(TARGET sccbase DESTINATION reactos/system32 FOR all)
