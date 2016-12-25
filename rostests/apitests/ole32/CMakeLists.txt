
set_cpp(WITH_RUNTIME)
add_executable(ole32_apitest initializespy.cpp testlist.c)
target_link_libraries(ole32_apitest wine uuid)
set_module_type(ole32_apitest win32cui)
add_importlibs(ole32_apitest user32 gdi32 shell32 ole32 shlwapi msvcrt kernel32)
add_cd_file(TARGET ole32_apitest DESTINATION reactos/bin FOR all)
