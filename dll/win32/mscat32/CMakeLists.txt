
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(mscat32.dll mscat32.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/mscat32.def)

add_library(mscat32 MODULE ${SOURCE})
set_module_type(mscat32 win32dll)
target_link_libraries(mscat32 wine)
add_importlibs(mscat32 wintrust msvcrt kernel32 ntdll)
add_cd_file(TARGET mscat32 DESTINATION reactos/system32 FOR all)
