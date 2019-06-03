
add_library(redirtest1 MODULE redirtest1.c)
spec2def(redirtest1 redirtest1.dll redirtest.spec ADD_IMPORTLIB)
set_module_type(redirtest1 win32dll)
add_importlibs(redirtest1 msvcrt kernel32 ntdll)
add_rostests_file(TARGET redirtest1 RENAME kernel32test_versioned.dll)

add_library(redirtest2 MODULE redirtest2.c)
spec2def(redirtest2 redirtest2.dll redirtest.spec ADD_IMPORTLIB)
set_module_type(redirtest2 win32dll)
add_importlibs(redirtest2 msvcrt kernel32 ntdll)
add_rostests_file(TARGET redirtest2 SUBDIR testdata RENAME kernel32test_versioned.dll)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/redirtest2.manifest" SUBDIR testdata)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/redir2dep.manifest" SUBDIR testdata)
