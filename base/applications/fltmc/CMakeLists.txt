
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

add_executable(fltmc fltmc.cpp fltmc.rc)
target_link_libraries(fltmc cpprt atl_classes)
set_target_cpp_properties(fltmc WITH_EXCEPTIONS)
set_module_type(fltmc win32cui UNICODE)
add_importlibs(fltmc fltlib msvcrt kernel32 advapi32)
add_cd_file(TARGET fltmc DESTINATION reactos/system32 FOR all)
