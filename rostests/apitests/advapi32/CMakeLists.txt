
list(APPEND SOURCE
    CreateService.c
    DuplicateTokenEx.c
    HKEY_CLASSES_ROOT.c
    LockDatabase.c
    QueryServiceConfig2.c
    RegEnumValueW.c
    RegQueryInfoKey.c
    RtlEncryptMemory.c
    SaferIdentifyLevel.c
    testlist.c)

add_executable(advapi32_apitest ${SOURCE})
target_link_libraries(advapi32_apitest wine ${PSEH_LIB})
set_module_type(advapi32_apitest win32cui)
add_importlibs(advapi32_apitest advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET advapi32_apitest DESTINATION reactos/bin FOR all)
