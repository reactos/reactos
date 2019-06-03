
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    main.c)

add_library(slbcsp MODULE ${SOURCE})
spec2def(slbcsp slbcsp.dll slbcsp.spec)

set_module_type(slbcsp win32dll)
target_link_libraries(slbcsp wine)
add_importlibs(slbcsp msvcrt kernel32 ntdll)
add_cd_file(TARGET slbcsp DESTINATION reactos/system32 FOR all)
