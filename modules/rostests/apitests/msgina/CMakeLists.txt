
add_executable(msgina_apitest
    ShellDimScreen.cpp
    testlist.c)

target_link_libraries(msgina_apitest wine uuid cpprt atl_classes)
set_module_type(msgina_apitest win32cui)
add_importlibs(msgina_apitest msvcrt user32 kernel32)
add_rostests_file(TARGET msgina_apitest)
