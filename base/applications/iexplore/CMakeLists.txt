
add_rc_deps(iexplore.rc ${CMAKE_CURRENT_SOURCE_DIR}/iexplore.ico)
add_executable(iexplore main.c iexplore.rc)
target_link_libraries(iexplore wine)
set_module_type(iexplore win32gui UNICODE)
add_delay_importlibs(iexplore advpack version user32)
add_importlibs(iexplore ieframe msvcrt kernel32 ntdll)
add_cd_file(TARGET iexplore DESTINATION reactos FOR all)
