
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    main.c)

add_library(sccbase MODULE ${SOURCE})
spec2def(sccbase sccbase.dll sccbase.spec)

set_module_type(sccbase win32dll)
target_link_libraries(sccbase wine)
add_importlibs(sccbase msvcrt kernel32 ntdll)
add_cd_file(TARGET sccbase DESTINATION reactos/system32 FOR all)
