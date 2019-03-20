
spec2def(msv1_0.dll msv1_0.spec)

list(APPEND SOURCE
    msv1_0.c
    msv1_0.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msv1_0_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msv1_0.def)

add_library(msv1_0 MODULE ${SOURCE})
set_module_type(msv1_0 win32dll UNICODE ENTRYPOINT 0)
target_link_libraries(msv1_0 wine ${PSEH_LIB})
add_delay_importlibs(msv1_0 samsrv lsasrv)
add_importlibs(msv1_0 advapi32 kernel32 ntdll)
add_dependencies(msv1_0 psdk)
add_cd_file(TARGET msv1_0 DESTINATION reactos/system32 FOR all)
