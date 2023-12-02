
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(url.dll url.spec)

list(APPEND SOURCE
    url_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/url_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/url.def)

list(APPEND url_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/res/102.ico)

set_source_files_properties(url.rc PROPERTIES OBJECT_DEPENDS "${url_rc_deps}")

add_library(url MODULE
    ${SOURCE}
    url.rc)

set_module_type(url win32dll)
target_link_libraries(url wine)
add_importlibs(url shell32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET url DESTINATION reactos/system32 FOR all)
