
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(timeout timeout.c timeout.rc)
set_module_type(timeout win32cui UNICODE)
target_link_libraries(timeout conutils ${PSEH_LIB})
add_importlibs(timeout msvcrt kernel32)
add_cd_file(TARGET timeout DESTINATION reactos/system32 FOR all)
