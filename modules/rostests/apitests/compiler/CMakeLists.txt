
list(APPEND SOURCE
    pseh.c
    pseh_cpp.cpp
    psehtest2.c
    testlist.c)

add_executable(compiler_apitest ${SOURCE})
target_link_libraries(compiler_apitest wine ${PSEH_LIB})
set_module_type(compiler_apitest win32cui)
add_importlibs(compiler_apitest msvcrt kernel32 ntdll)
add_rostests_file(TARGET compiler_apitest)
