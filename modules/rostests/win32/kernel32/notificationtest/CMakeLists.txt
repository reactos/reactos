
list(APPEND SOURCE
    notificationtest.c)

add_executable(notificationtest ${SOURCE})
target_link_libraries(notificationtest wine ${PSEH_LIB})
set_module_type(notificationtest win32cui)
add_importlibs(notificationtest gdi32 user32 shlwapi msvcrt kernel32 ntdll)
add_rostests_file(TARGET notificationtest SUBDIR suppl)
