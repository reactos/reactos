
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(tasklist tasklist.c tasklist.rc)
set_module_type(tasklist win32cui UNICODE)
target_link_libraries(tasklist conutils ${PSEH_LIB})
add_importlibs(tasklist msvcrt user32 kernel32 ntdll)
add_cd_file(TARGET tasklist DESTINATION reactos/system32 FOR all)
