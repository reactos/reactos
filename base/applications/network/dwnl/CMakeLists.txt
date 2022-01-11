
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

add_executable(dwnl dwnl.c dwnl.rc)
set_module_type(dwnl win32cui UNICODE)
target_link_libraries(dwnl conutils ${PSEH_LIB})
target_link_libraries(dwnl uuid)
add_importlibs(dwnl urlmon wininet msvcrt user32 kernel32)

if(MSVC)
    add_importlibs(dwnl ntdll)
endif()

add_cd_file(TARGET dwnl DESTINATION reactos/system32 FOR all)
