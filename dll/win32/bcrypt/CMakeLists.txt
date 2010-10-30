
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(bcrypt.dll bcrypt.spec)

add_library(bcrypt SHARED
    bcrypt_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/bcrypt.def)

set_module_type(bcrypt win32dll)

target_link_libraries(bcrypt wine)

add_importlibs(bcrypt kernel32 ntdll)


add_cab_target(bcrypt 1)
