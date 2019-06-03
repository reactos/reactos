
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    main.c)

add_library(initpki MODULE ${SOURCE})
spec2def(initpki initpki.dll initpki.spec)

set_module_type(initpki win32dll)
target_link_libraries(initpki wine)
add_importlibs(initpki msvcrt kernel32 ntdll)
add_cd_file(TARGET initpki DESTINATION reactos/system32 FOR all)
