
spec2def(shsvcs.dll shsvcs.spec ADD_IMPORTLIB)

add_library(shsvcs SHARED
    shsvcs.c
    thmsvc.c
    thmserver.c
    shsvcs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shsvcs_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shsvcs.def)

set_module_type(shsvcs win32dll UNICODE)
target_link_libraries(shsvcs wine)
add_importlibs(shsvcs uxtheme advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET shsvcs DESTINATION reactos/system32 FOR all)
