
add_definitions(-D__WINESRC__)
add_executable(reg export.c import.c reg.c reg.rc)
set_module_type(reg win32cui UNICODE TARGET_VERSION vista)
target_link_libraries(reg wine)
add_importlibs(reg advapi32 advapi32_vista user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET reg DESTINATION reactos/system32 FOR all)
