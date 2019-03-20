
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msisys.ocx msisys.ocx.spec)

list(APPEND SOURCE
    msisys.c
    ${CMAKE_CURRENT_BINARY_DIR}/msisys.def)

add_library(msisys MODULE ${SOURCE})
set_module_type(msisys win32ocx)
target_link_libraries(msisys wine)
add_importlibs(msisys msvcrt kernel32 ntdll)
add_cd_file(TARGET msisys DESTINATION reactos/system32 FOR all)
