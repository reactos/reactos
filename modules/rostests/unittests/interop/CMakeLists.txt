
PROJECT(interop_unittest)

include_directories(../../apitests/include)

list(APPEND SOURCE
    LocaleTests.cpp
    testlist.c)

add_executable(interop_unittest ${SOURCE})

target_link_libraries(interop_unittest cppstl)
add_cppstl_include_directories(interop_unittest)
set_module_type(interop_unittest win32cui)
target_compile_definitions(interop_unittest PRIVATE UNICODE _UNICODE _STLP_NO_EXCEPTIONS)
add_importlibs(interop_unittest shell32 user32 msvcrt kernel32)
add_rostests_file(TARGET interop_unittest)
