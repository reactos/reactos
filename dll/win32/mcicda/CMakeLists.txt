
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    mcicda.c)

add_library(mcicda MODULE ${SOURCE})
spec2def(mcicda mcicda.dll mcicda.spec)

set_module_type(mcicda win32dll)
target_link_libraries(mcicda wine)
add_importlibs(mcicda winmm user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mcicda DESTINATION reactos/system32 FOR all)
