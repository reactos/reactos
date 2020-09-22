
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msdmo.dll msdmo.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    dmoreg.c
    dmort.c)

list(APPEND PCH_SKIP_SOURCE
    stubs.c)

add_library(msdmo MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msdmo.def)

set_module_type(msdmo win32dll)
target_link_libraries(msdmo uuid wine mediaobj_guid)
add_importlibs(msdmo user32 advapi32 ole32 msvcrt kernel32 ntdll)
add_pch(msdmo precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET msdmo DESTINATION reactos/system32 FOR all)
