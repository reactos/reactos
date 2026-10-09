
spec2def(ncrypt.dll ncrypt.spec ADD_IMPORTLIB NO_PRIVATE_WARNINGS)

add_library(ncrypt MODULE
   main.c
    ${CMAKE_CURRENT_BINARY_DIR}/ncrypt.def
    ${CMAKE_CURRENT_BINARY_DIR}/ncrypt_stubs.c)

set_module_type(ncrypt win32dll)
target_link_libraries(ncrypt wine symcrypt oldnames)
add_importlibs(ncrypt bcrypt msvcrt advapi32 kernel32 kernel32_vista ntdll)
add_cd_file(TARGET ncrypt DESTINATION reactos/system32 FOR all)
set_wine_module(ncrypt)
