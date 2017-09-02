
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(clip clip.c clip.rc)
set_module_type(clip win32cui UNICODE)
target_link_libraries(clip conutils ${PSEH_LIB})
add_importlibs(clip advapi32 user32 msvcrt kernel32)
add_cd_file(TARGET clip DESTINATION reactos/system32 FOR all)
