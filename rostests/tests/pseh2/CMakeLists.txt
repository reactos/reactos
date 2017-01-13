
add_executable(pseh2_test psehtest.c psehtest2.c)
target_link_libraries(pseh2_test wine ${PSEH_LIB})
set_module_type(pseh2_test win32cui)
add_importlibs(pseh2_test msvcrt kernel32 ntdll)
add_rostests_file(TARGET pseh2_test)

if(NOT MSVC)
    add_target_compile_flags(pseh2_test "-Wno-format")
endif()


add_executable(pseh2_test_cpp psehtest_cpp.cpp psehtest2.c)
target_link_libraries(pseh2_test_cpp wine ${PSEH_LIB})
set_module_type(pseh2_test_cpp win32cui)
add_importlibs(pseh2_test_cpp msvcrt kernel32 ntdll)
add_rostests_file(TARGET pseh2_test_cpp)

if(NOT MSVC)
    add_target_compile_flags(pseh2_test_cpp "-Wno-format")
endif()
