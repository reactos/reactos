
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    dmoreg.c
    dmort.c
    precomp.h)

add_library(msdmo MODULE
    ${SOURCE}
    stubs.c
    rsrc.rc)
spec2def(msdmo msdmo.dll msdmo.spec ADD_IMPORTLIB)

set_module_type(msdmo win32dll)
target_link_libraries(msdmo uuid wine mediaobj_guid)
add_importlibs(msdmo user32 advapi32 ole32 msvcrt kernel32 ntdll)
add_pch(msdmo precomp.h SOURCE)
add_cd_file(TARGET msdmo DESTINATION reactos/system32 FOR all)
