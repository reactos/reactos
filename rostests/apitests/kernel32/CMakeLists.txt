
add_definitions(-D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    GetDriveType.c
    testlist.c)

add_executable(kernel32_apitest ${SOURCE})
target_link_libraries(kernel32_apitest wine ${PSEH_LIB})
set_module_type(kernel32_apitest win32cui)
add_importlibs(kernel32_apitest gdi32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET kernel32_apitest DESTINATION reactos/bin FOR all)
