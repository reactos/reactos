
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(at at.c at.rc)
set_module_type(at win32cui UNICODE)
target_link_libraries(at conutils ${PSEH_LIB})
add_importlibs(at msvcrt kernel32 user32 netapi32)
add_cd_file(TARGET at DESTINATION reactos/system32 FOR all)
