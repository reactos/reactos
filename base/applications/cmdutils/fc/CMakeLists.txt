include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(fc fc.c texta.c textw.c fc.rc)
set_module_type(fc win32cui UNICODE)
target_link_libraries(fc conutils wine ${PSEH_LIB})
add_importlibs(fc msvcrt shlwapi user32 kernel32 ntdll)
add_cd_file(TARGET fc DESTINATION reactos/system32 FOR all)
