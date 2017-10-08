
add_executable(binpatch patch.c)
set_module_type(binpatch win32cui)
add_importlibs(binpatch ntdll msvcrt kernel32)
if(MSVC)
    target_link_libraries(binpatch oldnames)
endif()
add_cd_file(TARGET binpatch DESTINATION reactos/bin FOR all)
