
add_subdirectory(ui)

list(APPEND SOURCE
    main.c
    ports.c
    precomp.h
    tools.c
    xcv.c)

add_library(localmon MODULE
    ${SOURCE}
    localmon.rc)
spec2def(localmon localmon.dll localmon.spec ADD_IMPORTLIB)

set_module_type(localmon win32dll UNICODE)
target_link_libraries(localmon wine)
add_importlibs(localmon advapi32 spoolss user32 msvcrt kernel32 ntdll)
add_pch(localmon precomp.h SOURCE)
add_cd_file(TARGET localmon DESTINATION reactos/system32 FOR all)
