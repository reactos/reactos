add_executable(gtest gtest.c)
set_module_type(gtest win32cui)
add_importlibs(gtest kernel32 msvcrt)
add_rostests_file(TARGET gtest)
