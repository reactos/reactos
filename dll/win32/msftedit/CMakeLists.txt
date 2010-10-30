
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msftedit ${CMAKE_CURRENT_SOURCE_DIR}/msftedit.spec ${CMAKE_CURRENT_BINARY_DIR}/msftedit.def)

add_library(msftedit SHARED
    msftedit_main.c
    version.rc)

set_module_type(msftedit win32dll)

target_link_libraries(msftedit
    ${CMAKE_CURRENT_BINARY_DIR}/msftedit.def
    uuid
    wine)

add_importlibs(msftedit riched20 kernel32 ntdll)

add_dependencies(msftedit msftedit_def)
add_cab_target(msftedit 1)
