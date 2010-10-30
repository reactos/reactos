
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(bcrypt ${CMAKE_CURRENT_SOURCE_DIR}/bcrypt.spec ${CMAKE_CURRENT_BINARY_DIR}/bcrypt.def)

add_library(bcrypt SHARED
    bcrypt_main.c
    version.rc)

set_module_type(bcrypt win32dll)

target_link_libraries(bcrypt
    ${CMAKE_CURRENT_BINARY_DIR}/bcrypt.def
    wine)

add_importlibs(bcrypt kernel32 ntdll)

add_dependencies(bcrypt bcrypt_def)
add_cab_target(bcrypt 1)
