

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msdmo.dll msdmo.spec)

add_library(msdmo SHARED
    dmoreg.c
    dmort.c
    stubs.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msdmo.def)

set_module_type(msdmo win32dll ENTRYPOINT 0 UNICODE)

target_link_libraries(msdmo uuid wine mediaobj_guid)

add_importlibs(msdmo
    user32
    advapi32
    ole32
    kernel32
    ntdll)

add_dependencies(msdmo dxsdk psdk)

add_cd_file(TARGET msdmo DESTINATION reactos/system32 FOR all)
