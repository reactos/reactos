
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
add_executable(reg add.c copy.c delete.c export.c import.c query.c reg.c reg.rc)
set_module_type(reg win32cui UNICODE)
target_link_libraries(reg wine oldnames)
add_importlibs(reg advapi32 advapi32_vista user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET reg DESTINATION reactos/system32 FOR all)
