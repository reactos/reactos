
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(msdmo.dll msdmo.spec)

list(APPEND SOURCE
    dmoreg.c
    dmort.c
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msdmo.def)

add_library(msdmo SHARED ${SOURCE} rsrc.rc)
set_module_type(msdmo win32dll)
target_link_libraries(msdmo uuid wine mediaobj_guid)
add_importlibs(msdmo user32 advapi32 ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msdmo DESTINATION reactos/system32 FOR all)
