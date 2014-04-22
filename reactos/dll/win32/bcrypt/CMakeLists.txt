
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(bcrypt.dll bcrypt.spec)

list(APPEND SOURCE
    bcrypt_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/bcrypt_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/bcrypt.def)

add_library(bcrypt SHARED ${SOURCE})
set_module_type(bcrypt win32dll)
target_link_libraries(bcrypt wine)
add_importlibs(bcrypt advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET bcrypt DESTINATION reactos/system32 FOR all)
