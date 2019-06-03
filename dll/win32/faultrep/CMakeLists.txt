
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    faultrep.c)

add_library(faultrep MODULE ${SOURCE})
spec2def(faultrep faultrep.dll faultrep.spec ADD_IMPORTLIB)

set_module_type(faultrep win32dll)
target_link_libraries(faultrep wine)
add_importlibs(faultrep advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET faultrep DESTINATION reactos/system32 FOR all)
