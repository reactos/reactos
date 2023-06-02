
add_rc_deps(conime.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/conime.ico)
add_executable(conime conime.c conime.rc)
target_link_libraries(conime wine ${PSEH_LIB})
set_module_type(conime win32gui UNICODE)
add_importlibs(conime ntdll gdi32 user32 advapi32 msvcrt kernel32 imm32)
add_pch(conime conime.h SOURCE)
add_cd_file(TARGET conime DESTINATION reactos/system32 FOR all)
