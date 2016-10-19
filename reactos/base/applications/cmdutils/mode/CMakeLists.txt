
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(mode mode.c mode.rc)
set_module_type(mode win32cui UNICODE)
target_link_libraries(mode conutils ${PSEH_LIB})
add_importlibs(mode user32 msvcrt kernel32)
set_target_properties(mode PROPERTIES SUFFIX ".com")
add_cd_file(TARGET mode DESTINATION reactos/system32 FOR all)
