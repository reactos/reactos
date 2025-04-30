
add_rc_deps(internat.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/internat.ico)
add_executable(internat internat.c internat.rc)
set_module_type(internat win32gui UNICODE)
add_importlibs(internat advapi32 imm32 user32 shell32 shlwapi gdi32 msvcrt kernel32)
add_cd_file(TARGET internat DESTINATION reactos/system32 FOR all)

add_subdirectory(indicdll)
