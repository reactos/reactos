
add_executable(shlextdbg shlextdbg.cpp shlextdbg.rc)

set_module_type(shlextdbg win32cui UNICODE)
target_link_libraries(shlextdbg uuid cpprt atl_classes)
add_importlibs(shlextdbg ole32 comctl32 shell32 shlwapi advapi32 user32 msvcrt kernel32)
add_cd_file(TARGET shlextdbg DESTINATION reactos/system32 FOR all)
