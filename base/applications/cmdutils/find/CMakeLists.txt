
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(find find.c find.rc)
set_module_type(find win32cui UNICODE)
target_link_libraries(find conutils ${PSEH_LIB})
add_importlibs(find user32 msvcrt kernel32 shlwapi)
add_cd_file(TARGET find DESTINATION reactos/system32 FOR all)
