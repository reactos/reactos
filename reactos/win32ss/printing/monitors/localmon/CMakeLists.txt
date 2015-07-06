
add_subdirectory(ui)

spec2def(localmon.dll localmon.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    ports.c
    precomp.h
    tools.c
    xcv.c)

add_library(localmon SHARED
    ${SOURCE}
    localmon.rc
    ${CMAKE_CURRENT_BINARY_DIR}/localmon.def)

set_module_type(localmon win32dll UNICODE)
target_link_libraries(localmon wine)
add_importlibs(localmon advapi32 spoolss user32 msvcrt kernel32 ntdll)
add_pch(localmon precomp.h SOURCE)
add_cd_file(TARGET localmon DESTINATION reactos/system32 FOR all)
