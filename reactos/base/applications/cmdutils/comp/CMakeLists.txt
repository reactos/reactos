
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(comp comp.c comp.rc)
set_module_type(comp win32cui UNICODE)
target_link_libraries(comp conutils ${PSEH_LIB})
add_importlibs(comp msvcrt kernel32)
add_cd_file(TARGET comp DESTINATION reactos/system32 FOR all)
