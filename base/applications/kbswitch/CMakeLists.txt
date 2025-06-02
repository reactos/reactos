
file(GLOB kbswitch_rc_deps res/*.*)
add_rc_deps(kbswitch.rc ${kbswitch_rc_deps})
add_executable(kbswitch kbswitch.c imemenu.c kbswitch.rc)
set_module_type(kbswitch win32gui UNICODE)
target_link_libraries(kbswitch wine)
add_importlibs(kbswitch advapi32 imm32 user32 shell32 shlwapi gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET kbswitch DESTINATION reactos/system32 FOR all)

add_subdirectory(indicdll)
<<<<<<< HEAD
=======
add_dependencies(kbswitch indicdll)
>>>>>>> 71ffb9a541d ([KBSWITCH][SDK][USER32] IME Menu and System Pen Icon)
