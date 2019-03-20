
spec2def(redirtest.dll redirtest.spec ADD_IMPORTLIB)

list(APPEND SOURCE1
    redirtest1.c
    ${CMAKE_CURRENT_BINARY_DIR}/redirtest_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/redirtest.def)

list(APPEND SOURCE2
    redirtest2.c
    ${CMAKE_CURRENT_BINARY_DIR}/redirtest_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/redirtest.def)

add_library(redirtest1 MODULE ${SOURCE1})
set_module_type(redirtest1 win32dll)
add_importlibs(redirtest1 msvcrt kernel32 ntdll)
add_rostests_file(TARGET redirtest1 RENAME kernel32test_versioned.dll)

add_library(redirtest2 MODULE ${SOURCE2})
set_module_type(redirtest2 win32dll)
add_importlibs(redirtest2 msvcrt kernel32 ntdll)
add_rostests_file(TARGET redirtest2 SUBDIR testdata RENAME kernel32test_versioned.dll)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/redirtest2.manifest" SUBDIR testdata)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/redir2dep.manifest" SUBDIR testdata)
