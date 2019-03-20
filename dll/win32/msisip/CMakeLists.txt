
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msisip.dll msisip.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/msisip_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msisip.def)

add_library(msisip MODULE ${SOURCE})
set_module_type(msisip win32dll)
target_link_libraries(msisip wine)
add_importlibs(msisip crypt32 ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msisip DESTINATION reactos/system32 FOR all)
