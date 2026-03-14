add_library(vf_test MODULE vf_test.c)
set_module_type(vf_test kernelmodedriver)
add_importlibs(vf_test ntoskrnl hal)