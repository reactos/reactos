
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(wintrust.dll wintrust.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    asn.c
    crypt.c
    register.c
    softpub.c
    wintrust_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wintrust_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wintrust.def)

add_library(wintrust SHARED ${SOURCE})
set_module_type(wintrust win32dll)
target_link_libraries(wintrust wine ${PSEH_LIB})
# FIXME: imagehlp should be delay-imported. See bug 7171
#add_delay_importlibs(wintrust cryptui imagehlp)
add_delay_importlibs(wintrust cryptui)
add_importlibs(wintrust imagehlp crypt32 user32 advapi32 msvcrt kernel32 ntdll)

add_cd_file(TARGET wintrust DESTINATION reactos/system32 FOR all)
