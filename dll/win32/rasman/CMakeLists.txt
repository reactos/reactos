
list(APPEND SOURCE
    rasman.c
    rasman.rc)

add_library(rasman MODULE ${SOURCE})
spec2def(rasman rasman.dll rasman.spec)

set_module_type(rasman win32dll)
target_link_libraries(rasman wine uuid)
add_importlibs(rasman msvcrt kernel32 ntdll)
add_cd_file(TARGET rasman DESTINATION reactos/system32 FOR all)
