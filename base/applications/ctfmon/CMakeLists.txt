
add_rc_deps(ctfmon.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/ctfmon.ico)
add_executable(ctfmon
    ctfmon.cpp
    CRegWatcher.cpp
    CTipBarWnd.cpp
    ctfmon.rc)
set_module_type(ctfmon win32gui UNICODE)
add_dependencies(ctfmon msctf)
target_link_libraries(ctfmon uuid)
add_importlibs(ctfmon msctf advapi32 imm32 user32 shell32 shlwapi gdi32 msvcrt kernel32)
add_cd_file(TARGET ctfmon DESTINATION reactos/system32 FOR all)
