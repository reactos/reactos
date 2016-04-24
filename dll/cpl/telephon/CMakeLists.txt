
spec2def(telephon.cpl telephon.spec)
add_rc_deps(telephon.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/applet.ico)

add_library(telephon SHARED
    telephon.c
    telephon.rc
    ${CMAKE_CURRENT_BINARY_DIR}/telephon.def)

set_module_type(telephon cpl UNICODE)
target_link_libraries(telephon uuid)
add_importlibs(telephon advapi32 user32 comctl32 ole32 shell32 msvcrt kernel32)
add_cd_file(TARGET telephon DESTINATION reactos/system32 FOR all)
