
spec2def(cryptbase.dll cryptbase.spec)

add_library(cryptbase MODULE
    cryptbase.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cryptbase.def)

set_module_type(cryptbase win32dll ENTRYPOINT 0)
add_importlibs(cryptbase advapi32 ntdll)
add_cd_file(TARGET cryptbase DESTINATION VerOS/system32 FOR all)
