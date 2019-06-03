
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    asn.c
    crypt.c
    register.c
    softpub.c
    wintrust_main.c
    precomp.h)

add_library(wintrust MODULE
    ${SOURCE}
    version.rc)
spec2def(wintrust wintrust.dll wintrust.spec ADD_IMPORTLIB)

set_module_type(wintrust win32dll)
target_link_libraries(wintrust wine ${PSEH_LIB})
# FIXME: imagehlp should be delay-imported. See CORE-6504
#add_delay_importlibs(wintrust cryptui imagehlp)
add_delay_importlibs(wintrust cryptui)
add_importlibs(wintrust imagehlp crypt32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(wintrust precomp.h SOURCE)
add_cd_file(TARGET wintrust DESTINATION reactos/system32 FOR all)
