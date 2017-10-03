
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(logoff logoff.c logoff.rc)
set_module_type(logoff win32cui UNICODE)
target_link_libraries(logoff conutils ${PSEH_LIB})
add_importlibs(logoff advapi32 user32 msvcrt kernel32)
add_cd_file(TARGET logoff DESTINATION reactos/system32 FOR all)
