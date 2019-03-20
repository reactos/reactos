
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(rsabase.dll rsabase.spec)

list(APPEND SOURCE
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/rsabase.def)

add_library(rsabase MODULE ${SOURCE})
set_module_type(rsabase win32dll ENTRYPOINT 0 )
target_link_libraries(rsabase wine)
add_importlibs(rsabase rsaenh ntdll)
add_cd_file(TARGET rsabase DESTINATION reactos/system32 FOR all)
