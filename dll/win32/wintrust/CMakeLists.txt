
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(wintrust ${CMAKE_CURRENT_SOURCE_DIR}/wintrust.spec ${CMAKE_CURRENT_BINARY_DIR}/wintrust.def)

add_library(wintrust SHARED
    crypt.c
    register.c
    wintrust_main.c
    asn.c
    softpub.c
    version.rc)

set_module_type(wintrust win32dll)

target_link_libraries(wintrust
    ${CMAKE_CURRENT_BINARY_DIR}/wintrust.def
    wine
    pseh)

add_importlibs(wintrust crypt32 cryptui user32 advapi32 imagehlp msvcrt kernel32 ntdll)
add_dependencies(wintrust wintrust_def)
add_cab_target(wintrust 1)
