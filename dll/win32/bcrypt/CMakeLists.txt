
add_definitions(-D__WINESRC__)
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/mbedtls)
spec2def(bcrypt.dll bcrypt.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    bcrypt_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/bcrypt_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/bcrypt.def)

add_library(bcrypt MODULE ${SOURCE})
set_module_type(bcrypt win32dll)
target_link_libraries(bcrypt wine)
add_importlibs(bcrypt mbedtls advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET bcrypt DESTINATION reactos/system32 FOR all)
