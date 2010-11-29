add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()
spec2def(msftedit.dll msftedit.spec)

list(APPEND SOURCE
    msftedit_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msftedit.def)

add_library(msftedit SHARED ${SOURCE})

set_module_type(msftedit win32dll)

target_link_libraries(msftedit
    uuid
    wine)

add_importlibs(msftedit riched20 msvcrt kernel32 ntdll)

add_cab_target(msftedit 1)
