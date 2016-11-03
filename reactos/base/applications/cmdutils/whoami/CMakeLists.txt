
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(whoami whoami.c whoami.rc)
set_module_type(whoami win32cui UNICODE)
target_link_libraries(whoami conutils ${PSEH_LIB})
add_importlibs(whoami user32 secur32 advapi32 msvcrt kernel32)
add_cd_file(TARGET whoami DESTINATION reactos/system32 FOR all)
