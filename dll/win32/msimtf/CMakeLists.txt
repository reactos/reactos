
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msimtf.dll msimtf.spec)

list(APPEND SOURCE
    activeimmapp.c
    main.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/msimtf_stubs.c)

add_library(msimtf MODULE
    ${SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msimtf.def)

set_module_type(msimtf win32dll)
target_link_libraries(msimtf uuid wine)
add_importlibs(msimtf imm32 msvcrt kernel32 ntdll)
add_pch(msimtf precomp.h SOURCE)
add_cd_file(TARGET msimtf DESTINATION reactos/system32 FOR all)
