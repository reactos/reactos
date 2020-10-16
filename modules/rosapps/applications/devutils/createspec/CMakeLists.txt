
add_executable(createspec createspec.c)
set_module_type(createspec win32cui)
target_link_libraries(createspec wine)
if(NOT MSVC)
    target_compile_options(createspec PRIVATE -Wno-stringop-overflow)
endif()
add_importlibs(createspec dbghelp msvcrt kernel32)
add_cd_file(TARGET createspec DESTINATION reactos/system32 FOR all)
