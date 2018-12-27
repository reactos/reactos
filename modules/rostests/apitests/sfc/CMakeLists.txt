
list(APPEND SOURCE
    SfcGetFiles.c
    SfcIsFileProtected.c
    testlist.c)

add_executable(sfc_apitest ${SOURCE})
set_module_type(sfc_apitest win32cui)
add_importlibs(sfc_apitest msvcrt user32 kernel32 ntdll)
add_rostests_file(TARGET sfc_apitest)
