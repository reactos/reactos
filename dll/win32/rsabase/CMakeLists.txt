
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    version.rc)

add_library(rsabase MODULE ${SOURCE})
spec2def(rsabase rsabase.dll rsabase.spec)

set_module_type(rsabase win32dll ENTRYPOINT 0)
target_link_libraries(rsabase wine)
add_importlibs(rsabase rsaenh ntdll)
add_cd_file(TARGET rsabase DESTINATION reactos/system32 FOR all)
