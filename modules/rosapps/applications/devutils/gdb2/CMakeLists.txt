
add_executable(gdb2 gdb2.cpp)
target_link_libraries(gdb2 cpprt)
set_module_type(gdb2 win32cui)
add_importlibs(gdb2 user32 msvcrt kernel32)
add_cd_file(TARGET gdb2 DESTINATION reactos/system32 FOR all)
