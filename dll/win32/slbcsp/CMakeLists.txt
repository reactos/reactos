
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(slbcsp.dll slbcsp.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/slbcsp_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/slbcsp.def)

add_library(slbcsp MODULE ${SOURCE})
set_module_type(slbcsp win32dll)
target_link_libraries(slbcsp wine)
add_importlibs(slbcsp msvcrt kernel32 ntdll)
add_cd_file(TARGET slbcsp DESTINATION reactos/system32 FOR all)
