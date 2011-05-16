
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()
spec2def(wintrust.dll wintrust.spec)

list(APPEND SOURCE
    crypt.c
    register.c
    wintrust_main.c
    asn.c
    softpub.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wintrust_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wintrust.def)

add_library(wintrust SHARED ${SOURCE})

set_module_type(wintrust win32dll)

target_link_libraries(wintrust
    wine
    ${PSEH_LIB})

add_importlibs(wintrust crypt32 cryptui user32 advapi32 imagehlp msvcrt kernel32 ntdll)

add_importlib_target(wintrust.spec)

add_cd_file(TARGET wintrust DESTINATION reactos/system32 FOR all)

