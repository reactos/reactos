
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

spec2def(normaliz.dll normaliz.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    IdnToAscii.c
    IdnToUnicode.c
    nameprep.c
    normalize.c
    ${CMAKE_CURRENT_BINARY_DIR}/normaliz.def)

add_library(normaliz MODULE ${SOURCE})
set_module_type(normaliz win32dll ENTRYPOINT 0)
add_importlibs(normaliz msvcrt kernel32 ntdll)
add_cd_file(TARGET normaliz DESTINATION reactos/system32 FOR all)
