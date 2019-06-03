
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    aes.c
    des.c
    handle.c
    implglue.c
    md2.c
    mpi.c
    rc2.c
    rc4.c
    rsa.c
    rsaenh.c
    sha2.c
    tomcrypt.h)

add_library(rsaenh MODULE
    ${SOURCE}
    rsrc.rc)
spec2def(rsaenh rsaenh.dll rsaenh.spec ADD_IMPORTLIB NO_PRIVATE_WARNINGS)

set_module_type(rsaenh win32dll)
target_link_libraries(rsaenh wine)
add_importlibs(rsaenh msvcrt crypt32 advapi32 kernel32 ntdll)
add_pch(rsaenh tomcrypt.h SOURCE)
add_cd_file(TARGET rsaenh DESTINATION reactos/system32 FOR all)
