
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msftedit.dll msftedit.spec)

list(APPEND SOURCE
    msftedit_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msftedit_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msftedit.def)

add_library(msftedit MODULE ${SOURCE})
set_module_type(msftedit win32dll)
target_link_libraries(msftedit uuid wine)
add_importlibs(msftedit riched20 msvcrt kernel32 ntdll)
add_cd_file(TARGET msftedit DESTINATION reactos/system32 FOR all)
