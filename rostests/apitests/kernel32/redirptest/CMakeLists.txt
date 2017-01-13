
spec2def(redirtest.dll redirtest.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    redirtest.c
    ${CMAKE_CURRENT_BINARY_DIR}/redirtest_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/redirtest.def)

add_definitions(-DTESTVER=1)

add_library(redirtest1 SHARED ${SOURCE})
set_module_type(redirtest1 win32dll)
add_importlibs(redirtest1 msvcrt kernel32 ntdll)
add_rostests_file(TARGET redirtest1 NAME_ON_CD kernel32test_versioned.dll)

remove_definitions(-DTESTVER=1)
add_definitions(-DTESTVER=2)

add_library(redirtest2 SHARED ${SOURCE})
set_module_type(redirtest2 win32dll)
add_importlibs(redirtest2 msvcrt kernel32 ntdll)
add_rostests_file(TARGET redirtest2 SUBDIR testdata NAME_ON_CD kernel32test_versioned.dll)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/redirtest2.manifest" SUBDIR testdata)

remove_definitions(-DTESTVER=2)