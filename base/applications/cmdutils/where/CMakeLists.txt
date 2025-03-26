include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(where where.c where.rc)
set_module_type(where win32cui UNICODE)
target_link_libraries(where conutils ${PSEH_LIB})
add_importlibs(where msvcrt kernel32)
add_cd_file(TARGET where DESTINATION reactos/system32 FOR all)
