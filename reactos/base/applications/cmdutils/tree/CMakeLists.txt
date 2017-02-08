
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(tree tree.c tree.rc)
set_module_type(tree win32cui UNICODE)
target_link_libraries(tree conutils ${PSEH_LIB})
add_importlibs(tree msvcrt kernel32)
set_target_properties(tree PROPERTIES SUFFIX ".com")
add_cd_file(TARGET tree DESTINATION reactos/system32 FOR all)
