
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msg711.acm msg711.acm.spec)

list(APPEND SOURCE
    msg711.c
    ${CMAKE_CURRENT_BINARY_DIR}/msg711.def)

add_library(msg711.acm MODULE ${SOURCE})
set_module_type(msg711.acm win32dll)
set_target_properties(msg711.acm PROPERTIES SUFFIX "")
target_link_libraries(msg711.acm wine)
add_importlibs(msg711.acm winmm user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msg711.acm DESTINATION reactos/system32 FOR all)
