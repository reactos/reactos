
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(url ${CMAKE_CURRENT_SOURCE_DIR}/url.spec ${CMAKE_CURRENT_BINARY_DIR}/url.def)

add_library(url SHARED url_main.c)

set_module_type(url win32dll)

target_link_libraries(url
    ${CMAKE_CURRENT_BINARY_DIR}/url.def
    wine)

add_importlibs(url shell32 shlwapi kernel32 ntdll)
add_dependencies(url url_def)
