
spec2def(indicdll.dll indicdll.spec)

file(GLOB indicdll_rc_deps res/*.*)
add_rc_deps(indicdll.rc ${indicdll_rc_deps})

list(APPEND SOURCE
    indicdll.c
    indicdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/indicdll.def)

add_library(indicdll MODULE ${SOURCE})
set_module_type(indicdll win32dll UNICODE)
target_link_libraries(indicdll wine)
add_importlibs(indicdll user32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET indicdll DESTINATION reactos/system32 FOR all)
