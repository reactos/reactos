
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(cacls cacls.c cacls.rc)
set_module_type(cacls win32cui UNICODE)
target_link_libraries(cacls conutils ${PSEH_LIB})
add_importlibs(cacls advapi32 user32 shell32 msvcrt kernel32)
add_cd_file(TARGET cacls DESTINATION reactos/system32 FOR all)
