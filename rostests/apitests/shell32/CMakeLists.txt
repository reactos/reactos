
set_cpp(WITH_RUNTIME)

list(APPEND SOURCE
   menu.cpp
   testlist.c)

add_executable(shell32_apitest ${SOURCE})
target_link_libraries(shell32_apitest wine uuid)
set_module_type(shell32_apitest win32cui)
add_importlibs(shell32_apitest msvcrt kernel32 user32 gdi32 shell32 ole32 shlwapi)
add_cd_file(TARGET shell32_apitest DESTINATION reactos/bin FOR all)
