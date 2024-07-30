
include_directories(../../apitests/include)

list(APPEND SOURCE
    LocaleTests.cpp
    testlist.c)

add_executable(interop_apitest ${SOURCE})

target_link_libraries(interop_apitest wine ${PSEH_LIB} cppstl)
set_module_type(interop_apitest win32cui)
target_compile_definitions(interop_apitest PRIVATE UNICODE _UNICODE _STLP_NO_EXCEPTIONS)
add_importlibs(interop_apitest shell32 user32 msvcrt kernel32)
add_rostests_file(TARGET interop_apitest)
