
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(replace replace.c util.c replace.rc)
set_module_type(replace win32cui UNICODE)
target_link_libraries(replace conutils ${PSEH_LIB})
add_importlibs(replace user32 msvcrt kernel32)
add_cd_file(TARGET replace DESTINATION reactos/system32 FOR all)
