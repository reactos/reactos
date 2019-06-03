
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    activeimmapp.c
    main.c
    precomp.h)

add_library(msimtf MODULE
    ${SOURCE}
    rsrc.rc)
spec2def(msimtf msimtf.dll msimtf.spec)

set_module_type(msimtf win32dll)
target_link_libraries(msimtf uuid wine)
add_importlibs(msimtf imm32 msvcrt kernel32 ntdll)
add_pch(msimtf precomp.h SOURCE)
add_cd_file(TARGET msimtf DESTINATION reactos/system32 FOR all)
