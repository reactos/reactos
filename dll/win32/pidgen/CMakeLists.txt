
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    main.c
    rsrc.rc)

add_library(pidgen MODULE ${SOURCE})
spec2def(pidgen pidgen.dll pidgen.spec)

set_module_type(pidgen win32dll)
target_link_libraries(pidgen wine)
add_importlibs(pidgen msvcrt kernel32 ntdll)
add_cd_file(TARGET pidgen DESTINATION reactos/system32 FOR all)
