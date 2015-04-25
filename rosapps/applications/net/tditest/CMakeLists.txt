
include_directories(include)
add_executable(tditest tditest/tditest.c tditest.rc)
set_module_type(tditest kernelmodedriver)
target_link_libraries(tditest ${PSEH_LIB})
add_importlibs(tditest ntoskrnl hal)
add_cd_file(TARGET tditest DESTINATION reactos/system32/drivers FOR all)
