
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(initpki.dll initpki.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/initpki_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/initpki.def)

add_library(initpki MODULE ${SOURCE})
set_module_type(initpki win32dll)
target_link_libraries(initpki wine)
add_importlibs(initpki msvcrt kernel32 ntdll)
add_cd_file(TARGET initpki DESTINATION reactos/system32 FOR all)
