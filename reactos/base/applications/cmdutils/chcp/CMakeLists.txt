
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(chcp chcp.c chcp.rc)
set_module_type(chcp win32cui UNICODE)
target_link_libraries(chcp conutils ${PSEH_LIB})
add_importlibs(chcp msvcrt kernel32)
set_target_properties(chcp PROPERTIES SUFFIX ".com")
add_cd_file(TARGET chcp DESTINATION reactos/system32 FOR all)
