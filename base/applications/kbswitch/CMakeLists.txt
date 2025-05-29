
add_rc_deps(kbswitch.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/kbswitch.ico)
add_executable(kbswitch kbswitch.c kbswitch.rc)
set_module_type(kbswitch win32gui UNICODE)
target_link_libraries(kbswitch wine)
add_importlibs(kbswitch advapi32 imm32 user32 shell32 shlwapi gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET kbswitch DESTINATION reactos/system32 FOR all)

add_subdirectory(kbsdll)
