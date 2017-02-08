
add_executable(dwnl dwnl.c)
set_module_type(dwnl win32cui UNICODE)
target_link_libraries(dwnl uuid)
add_importlibs(dwnl urlmon wininet msvcrt kernel32)

if(MSVC)
    add_importlibs(dwnl ntdll)
endif()

add_cd_file(TARGET dwnl DESTINATION reactos/system32 FOR all)
