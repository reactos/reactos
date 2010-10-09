
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(inseng ${CMAKE_CURRENT_SOURCE_DIR}/inseng.spec ${CMAKE_CURRENT_BINARY_DIR}/inseng.def)

add_library(inseng SHARED inseng_main.c regsvr.c)

set_module_type(inseng win32dll)

target_link_libraries(inseng
    ${CMAKE_CURRENT_BINARY_DIR}/inseng.def
    uuid
    wine)

add_importlibs(inseng
    ole32
    advapi32
    kernel32
    ntdll)

add_dependencies(inseng inseng_def)
