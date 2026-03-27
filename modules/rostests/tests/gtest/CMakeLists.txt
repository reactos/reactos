add_executable(gtest_vmm gtest.c)
set_module_type(gtest_vmm win32cui)
add_importlibs(gtest_vmm kernel32 msvcrt)
add_rostests_file(TARGET gtest_vmm)
