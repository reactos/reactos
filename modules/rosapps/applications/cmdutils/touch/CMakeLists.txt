
add_executable(touch err.c touch.c touch.rc)
target_link_libraries(touch getopt)
set_module_type(touch win32cui)
add_importlibs(touch msvcrt kernel32)
if(MSVC)
    target_link_libraries(touch oldnames)
endif()

add_cd_file(TARGET touch DESTINATION reactos/system32 FOR all)
