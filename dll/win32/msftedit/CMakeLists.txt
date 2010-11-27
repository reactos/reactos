
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msftedit.dll msftedit.spec)

add_library(msftedit SHARED
    msftedit_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msftedit.def)

set_module_type(msftedit win32dll)

target_link_libraries(msftedit
    uuid
    wine)

add_importlibs(msftedit riched20 msvcrt kernel32 ntdll)

add_cab_target(msftedit 1)
