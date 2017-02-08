
spec2def(joy.cpl joy.spec)
add_library(joy SHARED joy.c joy.rc ${CMAKE_CURRENT_BINARY_DIR}/joy.def)
set_module_type(joy cpl UNICODE)
target_link_libraries(joy uuid)

add_importlibs(joy
    user32
    comctl32
    ole32
    shell32
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET joy DESTINATION reactos/system32 FOR all)
