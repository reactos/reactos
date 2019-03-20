
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(olethk32.dll olethk32.spec)

list(APPEND SOURCE
    main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/olethk32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/olethk32.def)

add_library(olethk32 MODULE ${SOURCE})
set_module_type(olethk32 win32dll)
target_link_libraries(olethk32 wine)
add_importlibs(olethk32 msvcrt kernel32 ntdll)
add_cd_file(TARGET olethk32 DESTINATION reactos/system32 FOR all)
