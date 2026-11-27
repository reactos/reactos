add_library(vf_test MODULE vf_test.c)
set_module_type(vf_test kernelmodedriver)
add_importlibs(vf_test ntoskrnl hal)

add_executable(vf_query vf_query.c)
set_module_type(vf_query win32cui)
add_importlibs(vf_query msvcrt ntdll)
add_cd_file(TARGET vf_query DESTINATION reactos/system32 FOR all)