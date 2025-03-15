
add_executable(lnktool lnktool.cpp lnktool.rc)

set_module_type(lnktool win32cui UNICODE)
target_link_libraries(lnktool uuid)
add_importlibs(lnktool ole32 comctl32 shell32 shlwapi advapi32 user32 msvcrt kernel32)
add_cd_file(TARGET lnktool DESTINATION reactos/system32 FOR all)
