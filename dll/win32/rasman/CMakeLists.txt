
spec2def(rasman.dll rasman.spec)

list(APPEND SOURCE
    rasman.c
    rasman.rc
    ${CMAKE_CURRENT_BINARY_DIR}/rasman_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/rasman.def)

add_library(rasman MODULE ${SOURCE})
set_module_type(rasman win32dll)
target_link_libraries(rasman wine uuid)
add_importlibs(rasman msvcrt kernel32 ntdll)
add_cd_file(TARGET rasman DESTINATION reactos/system32 FOR all)
