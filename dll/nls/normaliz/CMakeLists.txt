spec2def(normaliz.dll normaliz.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    IdnToAscii.c
    IdnToUnicode.c
    nameprep.c
    normalize.c
    ${CMAKE_CURRENT_BINARY_DIR}/normaliz.def)

add_library(normaliz MODULE ${SOURCE})
set_module_type(normaliz win32dll ENTRYPOINT 0 TARGET_VERSION vista)
add_importlibs(normaliz msvcrt kernel32 ntdll)
add_cd_file(TARGET normaliz DESTINATION reactos/system32 FOR all)
