
add_executable(dwnl dwnl.c dwnl.rc)
set_module_type(dwnl win32cui UNICODE)
target_link_libraries(dwnl conutils uuid)
add_importlibs(dwnl urlmon wininet msvcrt kernel32)
if(MSVC AND ARCH STREQUAL "i386")
    add_importlibs(dwnl ntdll) # For _allmul
endif()
add_cd_file(TARGET dwnl DESTINATION reactos/system32 FOR all)
