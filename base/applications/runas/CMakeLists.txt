
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(runas runas.c runas.rc)
set_module_type(runas win32cui UNICODE)
target_link_libraries(runas conutils ${PSEH_LIB})
add_importlibs(runas advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET runas DESTINATION reactos/system32 FOR all)
