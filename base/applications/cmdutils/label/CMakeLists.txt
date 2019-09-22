
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(label label.c label.rc)
set_module_type(label win32cui UNICODE)
target_link_libraries(label conutils ${PSEH_LIB})
add_importlibs(label user32 msvcrt kernel32)
add_cd_file(TARGET label DESTINATION reactos/system32 FOR all)
