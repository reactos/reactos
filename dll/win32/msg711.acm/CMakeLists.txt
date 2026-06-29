
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
set_wine_module_FIXME(msg711.acm) # CORE-5743: No ARRAY_SIZE macro
