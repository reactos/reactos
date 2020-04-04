
spec2def(winprint.dll winprint.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    raw.c
    precomp.h)

add_library(winprint MODULE
    ${SOURCE}
    winprint.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winprint.def)

set_module_type(winprint win32dll UNICODE)
target_link_libraries(winprint wine)
add_importlibs(winprint spoolss msvcrt kernel32 ntdll)
add_pch(winprint precomp.h SOURCE)
if(ARCH STREQUAL "amd64")
    add_cd_file(TARGET winprint DESTINATION reactos/system32/spool/prtprocs/x64 FOR all)
else()
    add_cd_file(TARGET winprint DESTINATION reactos/system32/spool/prtprocs/w32x86 FOR all)
endif()
